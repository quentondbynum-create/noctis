#include "WavetableSynth.h"

const char* const NoctisSynthEngine::waveformNames[kNumWaveforms] =
{
    "Sine", "Saw", "Square", "Triangle",
    "808 Punch", "Dark Bell", "Supersaw", "Pluck"
};

//==============================================================================
NoctisSynthEngine::NoctisSynthEngine()
{
    buildTables();
    smoothCutoff.reset (44100.0, 0.02);
    smoothReso.reset   (44100.0, 0.02);
    smoothDrive.reset  (44100.0, 0.03);
    smoothChorus.reset (44100.0, 0.03);
    smoothGain.reset   (44100.0, 0.02);
}

void NoctisSynthEngine::Voice::reset (double sr)
{
    active = false; sustained = false; released = false;
    note = -1; age = 0; velocity = 0.0f;
    osc1Phase = osc2Phase = subPhase = 0.0;
    currentFreq = targetFreq = 440.0;
    stealFade = 1.0f;
    driftCents1 = driftCents2 = 0.0f;
    driftPhase1 = driftPhase2 = 0.0;
    driftRate1 = driftRate2 = 0.1;
    cutoffVar = envTimeVar = 1.0f;

    juce::dsp::ProcessSpec spec { sr, 512, 1 };
    ampEnv.setSampleRate (sr);
    filtEnv.setSampleRate (sr);
    filter.prepare (sr);
    filter.reset();
}

//==============================================================================
// Wavetable construction for the non-BLEP waves. Each is rendered at 8 mip
// levels with a decreasing harmonic ceiling so high notes never alias.
// (Saw/Square use polyBLEP at render time instead — even cleaner.)
void NoctisSynthEngine::renderTable (Waveform w, int maxHarmonic, std::vector<float>& out)
{
    out.assign (kTableSize, 0.0f);
    const double twoPi = juce::MathConstants<double>::twoPi;

    auto addHarmonic = [&] (int n, double amp, double phase = 0.0)
    {
        if (n < 1 || n > maxHarmonic) return;
        for (int i = 0; i < kTableSize; ++i)
        {
            const double ph = twoPi * n * i / kTableSize + phase;
            out[i] += (float) (amp * std::sin (ph));
        }
    };

    switch (w)
    {
        case kSine:
            addHarmonic (1, 1.0);
            break;

        case kSaw: // fallback table (polyBLEP preferred at render)
            for (int n = 1; n <= maxHarmonic; ++n)
                addHarmonic (n, 2.0 / (juce::MathConstants<double>::pi * n), juce::MathConstants<double>::pi);
            break;

        case kSquare:
            for (int n = 1; n <= maxHarmonic; n += 2)
                addHarmonic (n, 4.0 / (juce::MathConstants<double>::pi * n));
            break;

        case kTriangle:
            for (int n = 1, k = 0; n <= maxHarmonic; n += 2, ++k)
                addHarmonic (n, 8.0 / (juce::MathConstants<double>::pi * juce::MathConstants<double>::pi * n * n)
                                * ((k % 2 == 0) ? 1.0 : -1.0));
            break;

        case k808Punch: // punchy sine: strong fundamental + 2nd/3rd for knock
            addHarmonic (1, 1.0);
            addHarmonic (2, 0.45);
            addHarmonic (3, 0.18);
            addHarmonic (4, 0.06);
            break;

        case kDarkBell: // inharmonic bell partials (FM-style ratios)
        {
            const double ratios[] = { 1.0, 2.40, 3.90, 5.43, 6.79 };
            const double amps[]   = { 1.0, 0.55, 0.32, 0.20, 0.10 };
            for (int p = 0; p < 5; ++p)
            {
                if (ratios[p] > maxHarmonic && maxHarmonic < 32) continue;
                for (int i = 0; i < kTableSize; ++i)
                    out[i] += (float) (amps[p] * std::sin (twoPi * ratios[p] * i / kTableSize));
            }
            break;
        }

        case kSupersaw: // 7 detuned saws baked in
            for (int d = -3; d <= 3; ++d)
            {
                const double detune = 1.0 + d * 0.004;
                const double amp = (d == 0) ? 0.5 : 0.28;
                for (int n = 1; n <= maxHarmonic; ++n)
                {
                    const double a = amp * 2.0 / (juce::MathConstants<double>::pi * n);
                    for (int i = 0; i < kTableSize; ++i)
                        out[i] += (float) (a * std::sin (twoPi * n * detune * i / kTableSize + juce::MathConstants<double>::pi));
                }
            }
            break;

        case kPluck: // bright pluck: 1/n^1.4 decay, 2nd harmonic lift
            for (int n = 1; n <= maxHarmonic; ++n)
            {
                double a = 1.0 / std::pow (n, 1.4);
                if (n == 2) a *= 1.6;
                addHarmonic (n, a, juce::MathConstants<double>::pi);
            }
            break;

        default:
            addHarmonic (1, 1.0);
            break;
    }

    // Normalize to [-1, 1]
    float peak = 0.0f;
    for (auto s : out) peak = juce::jmax (peak, std::abs (s));
    if (peak > 1e-6f)
        for (auto& s : out) s /= peak;
}

void NoctisSynthEngine::buildTables()
{
    static constexpr int mipHarmonics[kNumMips] = { 128, 64, 32, 16, 8, 4, 2, 1 };
    tables.resize (kNumWaveforms);
    for (int w = 0; w < kNumWaveforms; ++w)
    {
        tables[w].resize (kNumMips);
        for (int m = 0; m < kNumMips; ++m)
            renderTable ((Waveform) w, mipHarmonics[m], tables[w][m]);
    }
}

const std::vector<float>& NoctisSynthEngine::tableFor (int wave, float freq) const
{
    wave = juce::jlimit (0, kNumWaveforms - 1, wave);
    const float nyquist = (float) sampleRate * 0.5f;
    static constexpr int mipHarmonics[kNumMips] = { 128, 64, 32, 16, 8, 4, 2, 1 };
    int best = kNumMips - 1;
    for (int m = 0; m < kNumMips; ++m)
    {
        if (mipHarmonics[m] * freq < nyquist * 0.9f) { best = m; break; }
    }
    return tables[wave][best];
}

//==============================================================================
void NoctisSynthEngine::prepare (double sr, int maxBlockSize)
{
    sampleRate = sr;
    for (auto& v : voices) v.reset (sr);

    smoothCutoff.reset (sr, 0.02);
    smoothReso.reset   (sr, 0.02);
    smoothDrive.reset  (sr, 0.03);
    smoothChorus.reset (sr, 0.03);
    smoothGain.reset   (sr, 0.02);

    driveOversampler.initProcessing ((size_t) maxBlockSize);
    driveOversampler.reset();
    tempMono.setSize (1, maxBlockSize);

    // effects buffers
    delayBufL.assign (kDelayMax, 0.0f);
    delayBufR.assign (kDelayMax, 0.0f);
    delayPos = 0; delayLastL = delayLastR = 0.0f;
    revComb1.fill (0); revComb2.fill (0); revComb3.fill (0); revComb4.fill (0);
    revAp1.fill (0); revAp2.fill (0);
    revPos1 = revPos2 = revPos3 = revPos4 = revApPos1 = revApPos2 = 0;
    crushHoldL = crushHoldR = 0.0f; crushCounter = 0;

    // reactive glow: crossover filters + envelope ballistics
    juce::dsp::ProcessSpec spec { sr, (juce::uint32) maxBlockSize, 1 };
    glowLow.coefficients  = juce::dsp::IIR::Coefficients<float>::makeLowPass (sr, 250.0f);
    glowMid.coefficients  = juce::dsp::IIR::Coefficients<float>::makeBandPass (sr, 1000.0f, 0.7f);
    glowHigh.coefficients = juce::dsp::IIR::Coefficients<float>::makeHighPass (sr, 4000.0f);
    glowLow.prepare (spec); glowMid.prepare (spec); glowHigh.prepare (spec);
    const float srF = (float) sr;
    glowAttackCoeff  = 1.0f - std::exp (-1.0f / (0.10f * srF)); // ~100ms
    glowReleaseCoeff = 1.0f - std::exp (-1.0f / (0.50f * srF)); // ~500ms
    glowEnvL = glowEnvM = glowEnvH = 0.0f;

    chorusBufL.fill (0.0f); chorusBufR.fill (0.0f);
    chorusPos = 0;
    peakFollowL = peakFollowR = 0.0f;
    peakLevelL.store (0.0f);
    peakLevelR.store (0.0f);
}

void NoctisSynthEngine::reset()
{
    for (auto& v : voices) { v.active = false; v.sustained = false; }
    sustainDown = false;
    driveOversampler.reset();
    chorusBufL.fill (0.0f); chorusBufR.fill (0.0f);
    glowLow.reset(); glowMid.reset(); glowHigh.reset();
    glowEnvL = glowEnvM = glowEnvH = 0.0f;
    glowR.store (0.545f); glowG.store (0.184f); glowB.store (0.969f);
}

void NoctisSynthEngine::updateReactiveGlow (float inL, float inR)
{
    const float in = (inL + inR) * 0.5f;
    const float lo = glowLow.processSample (in);
    const float mi = glowMid.processSample (in);
    const float hi = glowHigh.processSample (in);

    auto follow = [&] (float& env, float x)
    {
        const float ax = std::abs (x);
        const float c = (ax > env) ? glowAttackCoeff : glowReleaseCoeff;
        env += c * (ax - env);
    };
    follow (glowEnvL, lo);
    follow (glowEnvM, mi);
    follow (glowEnvH, hi);

    const float total = glowEnvL + glowEnvM + glowEnvH;
    float wL, wM, wH;
    if (total < 1e-4f) { wL = 1.0f; wM = 0.0f; wH = 0.0f; } // idle -> deep purple
    else { wL = glowEnvL / total; wM = glowEnvM / total; wH = glowEnvH / total; }

    // anchor colors: low=purple/magenta, mid=blue/cyan, high=electric cyan/white
    const float r = wL * 0.545f + wM * 0.000f + wH * 0.680f;
    const float g = wL * 0.184f + wM * 0.831f + wH * 0.961f;
    const float b = wL * 0.969f + wM * 1.000f + wH * 1.000f;
    glowR.store (r); glowG.store (g); glowB.store (b);
}

void NoctisSynthEngine::getActiveNotes (bool notes[128]) const noexcept
{
    for (int i = 0; i < 128; ++i) notes[i] = false;
    for (auto& v : voices)
        if (v.active && v.note >= 0 && v.note < 128)
            notes[v.note] = true;
}

//==============================================================================
NoctisSynthEngine::Voice* NoctisSynthEngine::findFreeVoice()
{
    for (auto& v : voices)
        if (! v.active) return &v;
    Voice* oldest = nullptr;
    for (auto& v : voices)
        if (v.released && (oldest == nullptr || v.age > oldest->age)) oldest = &v;
    if (oldest != nullptr) return oldest;
    oldest = &voices[0];
    for (auto& v : voices)
        if (v.age > oldest->age) oldest = &v;
    oldest->stealFade = 0.0f;
    return oldest;
}

void NoctisSynthEngine::noteOn (int note, float velocity)
{
    const float freq = midiToFreq (note);
    Voice* v = findFreeVoice();

    double startFreq = freq;
    if (params.glideSec > 0.001)
    {
        for (auto& other : voices)
        {
            if (&other != v && other.active && ! other.released)
            { startFreq = other.currentFreq; break; }
        }
    }

    v->active = true; v->sustained = false; v->released = false;
    v->note = note; v->velocity = velocity; v->age = 0;
    v->currentFreq = startFreq; v->targetFreq = freq;

    // --- analog voice variation (component tolerances) ---
    v->driftCents1 = (random.nextFloat() - 0.5f) * 14.0f; // +/-7 cents (hardware-like)
    v->driftCents2 = (random.nextFloat() - 0.5f) * 14.0f;
    v->driftPhase1 = random.nextDouble();
    v->driftPhase2 = random.nextDouble();
    v->driftRate1  = 0.05 + random.nextDouble() * 0.15;    // 0.05..0.2 Hz wander
    v->driftRate2  = 0.05 + random.nextDouble() * 0.15;
    v->cutoffVar   = 1.0f + (random.nextFloat() - 0.5f) * 0.08f; // +/-4% (component tolerance)
    v->envTimeVar  = 1.0f + (random.nextFloat() - 0.5f) * 0.10f; // +/-5%

    v->ampEnv.setParameters ({ params.ampA * v->envTimeVar, params.ampD * v->envTimeVar,
                               params.ampS, params.ampR * v->envTimeVar });
    v->filtEnv.setParameters ({ params.filtA * v->envTimeVar, params.filtD * v->envTimeVar,
                                params.filtS, params.filtR * v->envTimeVar });
    v->ampEnv.noteOn(); v->filtEnv.noteOn();

    v->filter.reset();

    for (auto& other : voices)
        if (&other != v) other.age++;
}

void NoctisSynthEngine::noteOff (int note)
{
    for (auto& v : voices)
    {
        if (v.active && ! v.released && v.note == note)
        {
            if (sustainDown) { v.sustained = true; }
            else { v.released = true; v.ampEnv.noteOff(); v.filtEnv.noteOff(); }
        }
    }
}

void NoctisSynthEngine::allNotesOff()
{
    for (auto& v : voices)
        if (v.active && ! v.released)
        { v.released = true; v.ampEnv.noteOff(); v.filtEnv.noteOff(); }
}

float NoctisSynthEngine::renderOsc (NoctisSynthEngine::Waveform wave, double& phase, double dt,
                                 const std::vector<float>& table) noexcept
{
    if (wave == NoctisSynthEngine::kSaw)
    {
        const double t = phase;
        double v = 2.0 * t - 1.0;
        v -= NoctisSynthEngine::polyBlep (t, dt);
        return (float) (v * 0.9);
    }
    if (wave == NoctisSynthEngine::kSquare)
    {
        const double t = phase;
        double v = (t < 0.5) ? 1.0 : -1.0;
        v += NoctisSynthEngine::polyBlep (t, dt);
        double t2 = t + 0.5; if (t2 >= 1.0) t2 -= 1.0;
        v -= NoctisSynthEngine::polyBlep (t2, dt);
        return (float) (v * 0.65);
    }
    return NoctisSynthEngine::readTable (table, phase);
}

//==============================================================================
void NoctisSynthEngine::process (juce::MidiBuffer& midi, juce::AudioBuffer<float>& audio)
{
    const int numSamples = audio.getNumSamples();
    const double twoPi = juce::MathConstants<double>::twoPi;

    // --- MIDI ---
    for (const auto meta : midi)
    {
        const auto msg = meta.getMessage();
        if (msg.isNoteOn())       noteOn (msg.getNoteNumber(), msg.getFloatVelocity());
        else if (msg.isNoteOff()) noteOff (msg.getNoteNumber());
        else if (msg.isSustainPedalOn())  sustainDown = true;
        else if (msg.isSustainPedalOff())
        {
            sustainDown = false;
            for (auto& v : voices)
                if (v.active && v.sustained)
                { v.sustained = false; v.released = true; v.ampEnv.noteOff(); v.filtEnv.noteOff(); }
        }
        else if (msg.isAllNotesOff()) allNotesOff();
    }

    // --- per-block parameter updates ---
    smoothCutoff.setTargetValue (juce::jlimit (20.0f, 20000.0f, params.cutoffHz));
    smoothReso.setTargetValue   (juce::jlimit (0.0f, 1.0f, params.resonance));
    smoothDrive.setTargetValue  (juce::jlimit (0.0f, 1.0f, params.drive));
    smoothChorus.setTargetValue (juce::jlimit (0.0f, 1.0f, params.chorus));
    smoothGain.setTargetValue   (juce::jlimit (0.0f, 1.0f, params.masterGain)
                                 * juce::jlimit (0.0f, 1.0f, params.masterGain) * 1.4f);

    const double glideCoeff = params.glideSec > 0.001
        ? std::exp (-1.0 / (params.glideSec * sampleRate)) : 0.0;

    const double lfoInc = twoPi * params.lfoRateHz / sampleRate;

    const float oscMix = juce::jlimit (0.0f, 1.0f, params.oscMix);
    const float subLevel = juce::jlimit (0.0f, 1.0f, params.subLevel);
    const float filtEnvAmt = juce::jlimit (-1.0f, 1.0f, params.filterEnvAmt);
    const float lfoDepth = juce::jlimit (0.0f, 1.0f, params.lfoDepth);
    const int osc1W = juce::jlimit (0, kNumWaveforms - 1, params.osc1Wave);
    const int osc2W = juce::jlimit (0, kNumWaveforms - 1, params.osc2Wave);

    // ==================================================================
    // STAGE 1: render voices (mono) into tempMono
    // ==================================================================
    float* mono = tempMono.getWritePointer (0);

    for (int s = 0; s < numSamples; ++s)
    {
        const float cutoffNow = smoothCutoff.getNextValue();
        const float resoNow   = smoothReso.getNextValue();
        const float driveNow  = smoothDrive.getNextValue();

        lfoPhase += lfoInc;
        if (lfoPhase >= twoPi) lfoPhase -= twoPi;
        const float lfo = (float) std::sin (lfoPhase);

        // ladder drive: 1.0 (clean) .. ~3.5 (saturated feedback path)
        const float ladderDrive = 1.0f + driveNow * 2.5f;
        // resonance curve: keep it musical — sings, doesn't scream
        const float resoMapped = std::pow (resoNow, 1.15f);

        float mix = 0.0f;

        for (auto& v : voices)
        {
            if (! v.active) continue;

            if (v.stealFade < 1.0f)
            {
                v.stealFade = juce::jmin (1.0f, v.stealFade + 1.0f / (0.005f * (float) sampleRate));
                if (v.stealFade <= 0.0f) continue;
            }

            if (glideCoeff > 0.0)
                v.currentFreq += (v.targetFreq - v.currentFreq) * (1.0 - glideCoeff);
            else
                v.currentFreq = v.targetFreq;

            float freq = (float) v.currentFreq;
            if (params.lfoTarget == 1) // pitch
                freq *= std::pow (2.0f, lfo * lfoDepth * 2.0f / 12.0f);

            // --- analog drift: per-osc detune + slow wander ---
            v.driftPhase1 += v.driftRate1 / sampleRate;
            if (v.driftPhase1 >= 1.0) v.driftPhase1 -= 1.0;
            v.driftPhase2 += v.driftRate2 / sampleRate;
            if (v.driftPhase2 >= 1.0) v.driftPhase2 -= 1.0;
            const float cents1 = v.driftCents1 + 2.5f * (float) std::sin (v.driftPhase1 * twoPi);
            const float cents2 = v.driftCents2 + 2.5f * (float) std::sin (v.driftPhase2 * twoPi);
            const float f1 = freq * std::pow (2.0f, cents1 / 1200.0f);
            const float f2 = freq * std::pow (2.0f, cents2 / 1200.0f);

            const double dt1 = f1 / sampleRate, dt2 = f2 / sampleRate;
            v.osc1Phase += dt1; if (v.osc1Phase >= 1.0) v.osc1Phase -= 1.0;
            v.osc2Phase += dt2; if (v.osc2Phase >= 1.0) v.osc2Phase -= 1.0;

            const auto& t1 = tableFor (osc1W, f1);
            const auto& t2 = tableFor (osc2W, f2);
            const float s1 = renderOsc ((Waveform) osc1W, v.osc1Phase, dt1, t1);
            const float s2 = renderOsc ((Waveform) osc2W, v.osc2Phase, dt2, t2);

            float osc = s1 * (1.0f - oscMix) + s2 * oscMix;

            // --- sub: pure sine, -1 octave, with real weight ---
            if (subLevel > 0.001f)
            {
                const double subDt = (freq * 0.5) / sampleRate;
                v.subPhase += subDt; if (v.subPhase >= 1.0) v.subPhase -= 1.0;
                float subOut;
                if (params.subWave == 1) // square sub: polyBLEP (no aliasing)
                {
                    const double t = v.subPhase;
                    double sq = (t < 0.5) ? 1.0 : -1.0;
                    sq += polyBlep (t, subDt);
                    double t2 = t + 0.5; if (t2 >= 1.0) t2 -= 1.0;
                    sq -= polyBlep (t2, subDt);
                    subOut = (float) (sq * 0.85);
                }
                else // pure sine: single harmonic, no aliasing possible
                {
                    subOut = (float) std::sin (v.subPhase * twoPi);
                }
                osc += subOut * subLevel * 1.5f;
            }

            // --- ladder filter: cutoff + env + LFO, per-voice variation ---
            float filtCut = cutoffNow * v.cutoffVar;
            if (std::abs (filtEnvAmt) > 0.001f)
            {
                const float envVal = v.filtEnv.getNextSample();
                filtCut *= std::pow (2.0f, envVal * filtEnvAmt * 4.0f); // +/-4 octaves
            }
            if (params.lfoTarget == 2) // filter
                filtCut *= std::pow (2.0f, lfo * lfoDepth * 2.0f);
            filtCut = juce::jlimit (20.0f, 20000.0f, filtCut);

            v.filter.setCutoff (filtCut);
            v.filter.setResonance (resoMapped);
            v.filter.setDrive (ladderDrive);

            float voiceOut = v.filter.process (osc);

            // --- amp env (+ tremolo) ---
            float ampEnvVal = v.ampEnv.getNextSample();
            if (params.lfoTarget == 3)
                ampEnvVal *= 1.0f - lfoDepth * 0.8f * (0.5f + 0.5f * lfo);

            voiceOut *= ampEnvVal * v.velocity * v.stealFade;
            mix += voiceOut;

            if (! v.ampEnv.isActive())
            { v.active = false; v.note = -1; }
        }

        mono[s] = mix * 0.30f; // voice sum headroom
    }

    // ==================================================================
    // STAGE 2: 2x oversampled tanh drive (no digital harshness)
    // ==================================================================
    {
        const float driveNow = smoothDrive.getCurrentValue();
        if (driveNow > 0.001f)
        {
            juce::dsp::AudioBlock<float> block (tempMono);
            block = block.getSubBlock (0, (size_t) numSamples);
            auto osBlock = driveOversampler.processSamplesUp (block);

            const float k = 1.0f + driveNow * 4.0f;
            const float norm = std::tanh (k * 0.5f);
            const size_t osLen = osBlock.getNumSamples();
            float* d = osBlock.getChannelPointer (0);
            for (size_t i = 0; i < osLen; ++i)
                d[i] = std::tanh (d[i] * k) / norm * 0.7f;

            driveOversampler.processSamplesDown (block);
        }
    }

    // ==================================================================
    // STAGE 3: chorus -> stereo, master gain, meters
    // ==================================================================
    float* outL = audio.getWritePointer (0);
    float* outR = audio.getNumChannels() > 1 ? audio.getWritePointer (1) : nullptr;

    const double chorusInc = twoPi * 0.6 / sampleRate;

    for (int s = 0; s < numSamples; ++s)
    {
        const float chorusNow = smoothChorus.getNextValue();
        const float gainNow   = smoothGain.getNextValue();
        const float mix = mono[s];

        float wetL = mix, wetR = mix;
        chorusLfoPhase += chorusInc;
        if (chorusLfoPhase >= twoPi) chorusLfoPhase -= twoPi;

        if (chorusNow > 0.001f)
        {
            const float srScale = (float) sampleRate / 44100.0f;
            const float baseDelay = 14.0f * srScale;
            const float depthSamp = 6.0f * srScale;
            const float dL = baseDelay + depthSamp * (0.5f + 0.5f * (float) std::sin (chorusLfoPhase));
            const float dR = baseDelay + depthSamp * (0.5f + 0.5f * (float) std::sin (chorusLfoPhase + juce::MathConstants<float>::pi));

            auto readDelay = [&] (const std::array<float, kChorusDelayMax>& buf, float d) -> float
            {
                float pos = (float) chorusPos - d;
                while (pos < 0) pos += kChorusDelayMax;
                const int i0 = ((int) pos) % kChorusDelayMax;
                const int i1 = (i0 + 1) % kChorusDelayMax;
                const float f = pos - (float) ((int) pos);
                return buf[i0] + f * (buf[i1] - buf[i0]);
            };
            const float cL = readDelay (chorusBufL, dL);
            const float cR = readDelay (chorusBufR, dR);
            chorusBufL[chorusPos] = mix;
            chorusBufR[chorusPos] = mix;
            chorusPos = (chorusPos + 1) % kChorusDelayMax;

            wetL = mix * (1.0f - chorusNow * 0.5f) + cL * chorusNow * 0.5f;
            wetR = mix * (1.0f - chorusNow * 0.5f) + cR * chorusNow * 0.5f;
        }

        wetL *= gainNow; wetR *= gainNow;

        // --- BITCRUSHER (lo-fi) ---
        const float crushNow = juce::jlimit (0.0f, 1.0f, params.bitcrush);
        if (crushNow > 0.001f)
        {
            // bit depth: 16 -> 4 bits, rate reduction: 1x -> 12x
            const int bits = juce::jmax (4, 16 - (int) (crushNow * 12.0f));
            const int rateDiv = 1 + (int) (crushNow * 11.0f);
            const float levels = (float) ((1 << bits) - 1);
            if ((crushCounter++ % rateDiv) == 0)
            {
                crushHoldL = std::round (juce::jlimit (-1.0f, 1.0f, wetL) * levels) / levels;
                crushHoldR = std::round (juce::jlimit (-1.0f, 1.0f, wetR) * levels) / levels;
            }
            const float dryMix = 1.0f - crushNow * 0.85f;
            wetL = wetL * dryMix + crushHoldL * (1.0f - dryMix);
            wetR = wetR * dryMix + crushHoldR * (1.0f - dryMix);
        }

        // --- PING-PONG DELAY ---
        const float dlyMix = juce::jlimit (0.0f, 1.0f, params.delayMix);
        if (dlyMix > 0.001f)
        {
            const int dlySamples = juce::jlimit (1, kDelayMax - 1,
                (int) (params.delayTimeMs * 0.001f * (float) sampleRate));
            const float fb = juce::jlimit (0.0f, 0.8f, params.delayFeedback);

            // read delayed (ping-pong: L reads from R's past and vice versa)
            const int readPos = (delayPos - dlySamples + kDelayMax * 2) % kDelayMax;
            const float dL = delayBufL[readPos];
            const float dR = delayBufR[readPos];

            // write with cross-feedback
            delayBufL[delayPos] = wetL + dR * fb;
            delayBufR[delayPos] = wetR + dL * fb;
            delayPos = (delayPos + 1) % kDelayMax;

            wetL = wetL * (1.0f - dlyMix * 0.5f) + dL * dlyMix * 0.5f;
            wetR = wetR * (1.0f - dlyMix * 0.5f) + dR * dlyMix * 0.5f;
        }

        // --- REVERB (Schroeder-Moorer, dark and lush) ---
        const float revMix = juce::jlimit (0.0f, 1.0f, params.reverb);
        if (revMix > 0.001f)
        {
            const float srScale = (float) sampleRate / 44100.0f;
            // comb delays (samples at 44.1k)
            const int c1 = (int) (1116 * srScale), c2 = (int) (1188 * srScale);
            const int c3 = (int) (1277 * srScale), c4 = (int) (1356 * srScale);
            const int a1 = (int) (225 * srScale), a2 = (int) (556 * srScale);
            const float combFb = 0.82f; // decay
            const float damp = 0.35f;   // dark

            auto comb = [&] (std::array<float, kReverbCombMax>& buf, int& pos, int len, float in) -> float
            {
                float out = buf[pos];
                buf[pos] = in + out * combFb;
                // simple lowpass in feedback for darkness
                buf[pos] = buf[pos] * (1.0f - damp) + out * damp * 0.5f;
                pos = (pos + 1) % len;
                return out;
            };
            auto allpass = [&] (std::array<float, kReverbApMax>& buf, int& pos, int len, float in) -> float
            {
                float bufOut = buf[pos];
                float out = -in + bufOut;
                buf[pos] = in + bufOut * 0.5f;
                pos = (pos + 1) % len;
                return out;
            };

            float monoIn = (wetL + wetR) * 0.5f;
            float r = comb (revComb1, revPos1, c1, monoIn)
                    + comb (revComb2, revPos2, c2, monoIn)
                    + comb (revComb3, revPos3, c3, monoIn)
                    + comb (revComb4, revPos4, c4, monoIn);
            r *= 0.25f;
            r = allpass (revAp1, revApPos1, a1, r);
            r = allpass (revAp2, revApPos2, a2, r);

            wetL = wetL * (1.0f - revMix * 0.6f) + r * revMix * 0.6f;
            wetR = wetR * (1.0f - revMix * 0.6f) + r * revMix * 0.6f;
        }

        // Vintage master saturation — subtle analog glue, always on.
        // Gentle tanh curve adds harmonic warmth like recording through hardware.
        wetL = std::tanh (wetL * 1.15f) * 0.92f;
        wetR = std::tanh (wetR * 1.15f) * 0.92f;

        wetL = juce::jlimit (-1.2f, 1.2f, wetL);
        wetR = juce::jlimit (-1.2f, 1.2f, wetR);
        if (std::abs (wetL) < 1e-9f) wetL = 0.0f;
        if (std::abs (wetR) < 1e-9f) wetR = 0.0f;

        outL[s] = wetL;
        if (outR != nullptr) outR[s] = wetR;

        updateReactiveGlow (wetL, wetR);

        peakFollowL = juce::jmax (std::abs (wetL), peakFollowL * 0.9995f);
        peakFollowR = juce::jmax (std::abs (wetR), peakFollowR * 0.9995f);
        if ((s & 63) == 0)
        {
            peakLevelL.store (peakFollowL);
            peakLevelR.store (peakFollowR);
        }
    }
}
