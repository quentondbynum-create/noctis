#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <vector>
#include <cmath>

//==============================================================================
/*
    NOCTIS wavetable synth engine — ANALOG CHARACTER build.

    16-voice polyphonic synth voiced like real hardware:
      2 oscillators/voice:
        - Saw & Square: polyBLEP (true zero-aliasing, like analog VCOs)
        - Sine, Triangle, 808 Punch, Dark Bell, Supersaw, Pluck:
          precomputed mip-mapped wavetables (bandlimited, no aliasing)
      Per-oscillator analog drift: random +/-4 cents + slow wander LFO
      1 sub oscillator (-1 octave, pure sine computed directly, heavy)
      1 Moog-style ladder filter (24dB) with drive/saturation in the
        feedback path — resonance sings instead of screaming digitally
      Amp ADSR + Filter ADSR, 1 LFO (pitch/filter/amp)
      Per-voice variation: filter cutoff +/-2%, envelope timing +/-5%
        (round-robin randomization at note-on, like component tolerances)
      Glide/portamento
      Global drive stage: tanh waveshaper at 2x oversampling (no digital
        harshness) + simple chorus, smoothed master gain

    All DSP is click-free: ADSRs handle note boundaries, SmoothedValues guard
    parameter changes, voice stealing uses a fast fade.
*/
class NoctisSynthEngine
{
public:
    static constexpr int kNumVoices = 16;
    static constexpr int kTableSize  = 2048;
    static constexpr int kNumMips    = 8;

    // Waveform indices (must match UI + presets)
    enum Waveform
    {
        kSine = 0, kSaw, kSquare, kTriangle,
        k808Punch, kDarkBell, kSupersaw, kPluck,
        kNumWaveforms
    };

    static const char* const waveformNames[kNumWaveforms];

    NoctisSynthEngine();
    ~NoctisSynthEngine() = default;

    void prepare (double sampleRate, int maxBlockSize);
    void reset();

    // --- live parameters (set from APVTS each block, smoothed internally) ---
    struct Params
    {
        int   osc1Wave = kSaw, osc2Wave = kSaw;
        float oscMix = 0.5f;          // 0 = osc1 only, 1 = osc2 only
        float subLevel = 0.0f;        // 0..1
        int   subWave = 0;            // 0 = sine, 1 = square
        float cutoffHz = 8000.0f;
        float resonance = 0.1f;       // 0..1
        float filterEnvAmt = 0.0f;    // -1..1
        // amp ADSR
        float ampA = 0.005f, ampD = 0.1f, ampS = 0.8f, ampR = 0.15f;
        // filter ADSR
        float filtA = 0.005f, filtD = 0.1f, filtS = 0.5f, filtR = 0.15f;
        int   lfoTarget = 1;          // 0=off, 1=pitch, 2=filter, 3=amp
        float lfoRateHz = 5.0f;
        float lfoDepth = 0.0f;        // 0..1
        float glideSec = 0.0f;
        float drive = 0.0f;           // 0..1
        float chorus = 0.0f;          // 0..1
        float masterGain = 0.8f;      // 0..1
    };

    void setParams (const Params& p) { params = p; }

    // MIDI + audio
    void process (juce::MidiBuffer& midi, juce::AudioBuffer<float>& audio);

    // Peak levels for VU meters (0..1) — stereo
    float getPeakLevelL() const noexcept { return peakLevelL.load(); }
    float getPeakLevelR() const noexcept { return peakLevelR.load(); }
    float getPeakLevel() const noexcept
    {
        return juce::jmax (peakLevelL.load(), peakLevelR.load());
    }

    // Audio-reactive glow color (message-thread safe)
    juce::Colour getGlowColour() const noexcept
    {
        return juce::Colour::fromFloatRGBA (glowR.load(), glowG.load(), glowB.load(), 1.0f);
    }

    // Active notes for keyboard highlight (128 bools, true = sounding)
    void getActiveNotes (bool notes[128]) const noexcept;

private:
    // --- polyBLEP helpers (bandlimited classic analog waves) ---
    static inline double polyBlep (double t, double dt) noexcept
    {
        // t = phase in [0,1), dt = phase increment per sample
        if (t < dt)
        {
            t /= dt;
            return t + t - t * t - 1.0;
        }
        if (t > 1.0 - dt)
        {
            t = (t - 1.0) / dt;
            return t * t + t + t + 1.0;
        }
        return 0.0;
    }

    // --- wavetables (for sine/triangle/trap tables — already bandlimited) ---
    // tables[wave][mip][sample]
    std::vector<std::vector<std::vector<float>>> tables;

    void buildTables();
    void renderTable (Waveform w, int maxHarmonic, std::vector<float>& out);
    const std::vector<float>& tableFor (int wave, float freq) const;

    // table lookup with linear interpolation; phase in [0,1)
    static inline float readTable (const std::vector<float>& t, double phase) noexcept
    {
        const double idx = phase * kTableSize;
        const int i0 = (int) idx;
        const float frac = (float) (idx - i0);
        const int i1 = (i0 + 1) % kTableSize;
        return t[i0] + frac * (t[i1] - t[i0]);
    }

    // Render one oscillator sample. Saw/Square use polyBLEP (zero aliasing);
    // everything else reads the mip-mapped tables. phase in [0,1).
    static float renderOsc (Waveform wave, double& phase, double dt,
                            const std::vector<float>& table) noexcept;

    // --- analog Moog-style 24dB ladder filter (per-sample, nonlinear) ---
    // Huovilainen-inspired: 4 cascaded one-poles with tanh saturation per
    // stage + feedback. Drive saturates the input, resonance feeds back from
    // the output — resonance sings instead of screaming digitally.
    struct MoogLadder
    {
        void prepare (double sr) noexcept
        {
            sampleRate = sr;
            reset();
        }

        void reset() noexcept
        {
            s1 = s2 = s3 = s4 = 0.0f;
        }

        void setCutoff (float hz) noexcept
        {
            // stable tuning: g in (0,1); maps 20Hz..20kHz musically
            const float fc = juce::jlimit (20.0f, 20000.0f, hz) / (float) sampleRate;
            g = 1.0f - std::exp (-2.0f * juce::MathConstants<float>::pi * fc);
        }

        void setResonance (float r) noexcept // 0..1
        {
            // feedback gain; 4.0 = self-oscillation, stay just under
            res = juce::jlimit (0.0f, 1.0f, r) * 3.9f;
        }

        void setDrive (float d) noexcept // 1.0 = unity
        {
            drive = juce::jmax (0.1f, d);
        }

        inline float process (float in) noexcept
        {
            // input saturation (the drive)
            const float x = std::tanh (in * drive * 0.5f) * 2.0f - res * s4;
            // 4 nonlinear one-pole stages
            s1 += g * (std::tanh (x)  - std::tanh (s1));
            s2 += g * (std::tanh (s1) - std::tanh (s2));
            s3 += g * (std::tanh (s2) - std::tanh (s3));
            s4 += g * (std::tanh (s3) - std::tanh (s4));
            return s4;
        }

    private:
        double sampleRate = 44100.0;
        float g = 0.1f, res = 0.0f, drive = 1.0f;
        float s1 = 0.0f, s2 = 0.0f, s3 = 0.0f, s4 = 0.0f;
    };

    // --- voice ---
    struct Voice
    {
        bool active = false;
        bool sustained = false;   // held by sustain pedal
        bool released = false;
        int  note = -1;
        int  age = 0;             // for voice stealing (oldest)
        float velocity = 0.0f;

        double osc1Phase = 0.0, osc2Phase = 0.0, subPhase = 0.0; // [0,1)
        double currentFreq = 440.0, targetFreq = 440.0; // glide
        float  stealFade = 1.0f;  // fast fade when stolen

        // analog drift: per-osc random detune + slow wander
        float driftCents1 = 0.0f, driftCents2 = 0.0f; // +/-4 cents at note-on
        double driftPhase1 = 0.0, driftPhase2 = 0.0;  // slow LFO phase
        double driftRate1 = 0.1, driftRate2 = 0.1;    // Hz, randomized

        // component-tolerance variation
        float cutoffVar = 1.0f;   // +/-2%
        float envTimeVar = 1.0f;  // +/-5%

        juce::ADSR ampEnv, filtEnv;
        MoogLadder filter; // analog 24dB ladder, per-sample nonlinear

        void reset (double sampleRate);
    };
    std::array<Voice, kNumVoices> voices;

    void noteOn (int note, float velocity);
    void noteOff (int note);
    void allNotesOff();
    Voice* findFreeVoice();

    // --- global state ---
    double sampleRate = 44100.0;
    Params params;
    juce::Random random; // voice variation (not realtime-critical: note-on only)

    // Smoothed "performance" values (cutoff/resonance also driven by XY pad)
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothCutoff, smoothReso;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothDrive, smoothChorus, smoothGain;

    // LFO
    double lfoPhase = 0.0;

    // 2x oversampled drive stage (tanh) — kills digital harshness
    juce::dsp::Oversampling<float> driveOversampler {
        1, 1,
        juce::dsp::Oversampling<float>::FilterType::filterHalfBandPolyphaseIIR,
        true, false
    };
    juce::AudioBuffer<float> tempMono; // voice mix before drive

    // Chorus: two short modulated delays
    static constexpr int kChorusDelayMax = 2048;
    std::array<float, kChorusDelayMax> chorusBufL {}, chorusBufR {};
    int chorusPos = 0;
    double chorusLfoPhase = 0.0;

    // Sustain pedal
    bool sustainDown = false;

    // --- audio-reactive glow: 3-band spectral analysis -> RGB ---
    // Lows -> deep purple/magenta, mids -> blue/cyan, highs -> electric cyan/white.
    // Envelopes use ~100ms attack / ~500ms release for fluid (non-flickery) motion.
    // Silent/idle falls back to deep purple.
    juce::dsp::IIR::Filter<float> glowLow, glowMid, glowHigh;
    float glowEnvL = 0.0f, glowEnvM = 0.0f, glowEnvH = 0.0f;
    float glowAttackCoeff = 0.0f, glowReleaseCoeff = 0.0f;
    std::atomic<float> glowR { 0.545f }, glowG { 0.184f }, glowB { 0.969f };
    void updateReactiveGlow (float inL, float inR);

    // Peak followers for meters (stereo)
    std::atomic<float> peakLevelL { 0.0f };
    std::atomic<float> peakLevelR { 0.0f };
    float peakFollowL = 0.0f, peakFollowR = 0.0f;

    static float midiToFreq (int note) noexcept
    {
        return 440.0f * std::pow (2.0f, (note - 69) / 12.0f);
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NoctisSynthEngine)
};
