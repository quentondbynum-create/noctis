#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
NoctisProcessor::NoctisProcessor()
    : AudioProcessor (BusesProperties()
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "NoctisParams", createParameterLayout())
{
}

NoctisProcessor::~NoctisProcessor() = default;

juce::AudioProcessorValueTreeState::ParameterLayout NoctisProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    juce::StringArray waveNames;
    for (int i = 0; i < NoctisSynthEngine::kNumWaveforms; ++i)
        waveNames.add (NoctisSynthEngine::waveformNames[i]);

    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        kOsc1Wave, "Osc 1 Wave", waveNames, 1)); // default Saw
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        kOsc2Wave, "Osc 2 Wave", waveNames, 1));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        kOscMix, "Osc Mix",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 50.0f,
        juce::AudioParameterFloatAttributes().withLabel ("%")));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        kSubLevel, "Sub Level",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 0.0f,
        juce::AudioParameterFloatAttributes().withLabel ("%")));

    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        kSubWave, "Sub Wave",
        juce::StringArray ({ "Sine", "Square" }), 0));

    juce::NormalisableRange<float> cutoffRange (20.0f, 20000.0f);
    cutoffRange.setSkewForCentre (800.0f);
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        kCutoff, "Cutoff", cutoffRange, 8000.0f,
        juce::AudioParameterFloatAttributes().withLabel ("Hz")));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        kResonance, "Resonance",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 10.0f,
        juce::AudioParameterFloatAttributes().withLabel ("%")));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        kFilterEnv, "Filter Env Amount",
        juce::NormalisableRange<float> (-100.0f, 100.0f, 0.1f), 0.0f,
        juce::AudioParameterFloatAttributes().withLabel ("%")));

    auto msRange = [] { juce::NormalisableRange<float> r (1.0f, 2000.0f); r.setSkewForCentre (100.0f); return r; };
    auto msRangeR = [] { juce::NormalisableRange<float> r (10.0f, 5000.0f); r.setSkewForCentre (300.0f); return r; };

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        kAmpA, "Amp Attack", msRange(), 5.0f,
        juce::AudioParameterFloatAttributes().withLabel ("ms")));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        kAmpD, "Amp Decay", msRange(), 100.0f,
        juce::AudioParameterFloatAttributes().withLabel ("ms")));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        kAmpS, "Amp Sustain",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 80.0f,
        juce::AudioParameterFloatAttributes().withLabel ("%")));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        kAmpR, "Amp Release", msRangeR(), 150.0f,
        juce::AudioParameterFloatAttributes().withLabel ("ms")));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        kFiltA, "Filter Attack", msRange(), 5.0f,
        juce::AudioParameterFloatAttributes().withLabel ("ms")));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        kFiltD, "Filter Decay", msRange(), 100.0f,
        juce::AudioParameterFloatAttributes().withLabel ("ms")));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        kFiltS, "Filter Sustain",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 50.0f,
        juce::AudioParameterFloatAttributes().withLabel ("%")));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        kFiltR, "Filter Release", msRangeR(), 150.0f,
        juce::AudioParameterFloatAttributes().withLabel ("ms")));

    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        kLfoTarget, "LFO Target",
        juce::StringArray ({ "Off", "Pitch", "Filter", "Amp" }), 0));

    juce::NormalisableRange<float> lfoRange (0.05f, 20.0f);
    lfoRange.setSkewForCentre (2.0f);
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        kLfoRate, "LFO Rate", lfoRange, 5.0f,
        juce::AudioParameterFloatAttributes().withLabel ("Hz")));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        kLfoDepth, "LFO Depth",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 0.0f,
        juce::AudioParameterFloatAttributes().withLabel ("%")));

    juce::NormalisableRange<float> glideRange (0.0f, 500.0f);
    glideRange.setSkewForCentre (80.0f);
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        kGlide, "Glide", glideRange, 0.0f,
        juce::AudioParameterFloatAttributes().withLabel ("ms")));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        kDrive, "Drive",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 0.0f,
        juce::AudioParameterFloatAttributes().withLabel ("%")));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        kChorus, "Chorus",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 0.0f,
        juce::AudioParameterFloatAttributes().withLabel ("%")));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        kMasterGain, "Master Gain",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 80.0f,
        juce::AudioParameterFloatAttributes().withLabel ("%")));

    return { params.begin(), params.end() };
}

//==============================================================================
void NoctisProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    synth.prepare (sampleRate, samplesPerBlock);
    // 2x oversampling adds a few samples of latency; inaudible for a synth,
    // but report honestly.
    setLatencySamples (0);
}

void NoctisProcessor::releaseResources() {}

bool NoctisProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    // Synth: no audio input, stereo output.
    if (layouts.getMainInputChannelSet() != juce::AudioChannelSet::disabled())
        return false;
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void NoctisProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    // Drain UI-thread MIDI (on-screen keyboard) into this block's buffer.
    {
        juce::ScopedLock sl (midiQueueLock);
        for (auto& m : midiQueue)
            midiMessages.addEvent (m, 0);
        midiQueue.clear();
    }

    // Pull APVTS -> engine params (per block; engine smooths internally)
    NoctisSynthEngine::Params p;
    auto getFloat = [&] (const char* id) {
        return apvts.getRawParameterValue (id)->load();
    };
    auto getChoice = [&] (const char* id) {
        return (int) apvts.getRawParameterValue (id)->load();
    };

    p.osc1Wave   = getChoice (kOsc1Wave);
    p.osc2Wave   = getChoice (kOsc2Wave);
    p.oscMix     = getFloat (kOscMix) / 100.0f;
    p.subLevel   = getFloat (kSubLevel) / 100.0f;
    p.subWave    = getChoice (kSubWave);
    p.cutoffHz   = getFloat (kCutoff);
    p.resonance  = getFloat (kResonance) / 100.0f;
    p.filterEnvAmt = getFloat (kFilterEnv) / 100.0f;
    p.ampA = getFloat (kAmpA) / 1000.0f;
    p.ampD = getFloat (kAmpD) / 1000.0f;
    p.ampS = getFloat (kAmpS) / 100.0f;
    p.ampR = getFloat (kAmpR) / 1000.0f;
    p.filtA = getFloat (kFiltA) / 1000.0f;
    p.filtD = getFloat (kFiltD) / 1000.0f;
    p.filtS = getFloat (kFiltS) / 100.0f;
    p.filtR = getFloat (kFiltR) / 1000.0f;
    p.lfoTarget  = getChoice (kLfoTarget);
    p.lfoRateHz  = getFloat (kLfoRate);
    p.lfoDepth   = getFloat (kLfoDepth) / 100.0f;
    p.glideSec   = getFloat (kGlide) / 1000.0f;
    p.drive      = getFloat (kDrive) / 100.0f;
    p.chorus     = getFloat (kChorus) / 100.0f;
    p.masterGain = getFloat (kMasterGain) / 100.0f;

    synth.setParams (p);
    synth.process (midiMessages, buffer);
}

//==============================================================================
void NoctisProcessor::applyPreset (const NoctisPreset& preset)
{
    auto setFloat = [&] (const char* id, float value) {
        if (auto* param = apvts.getParameter (id))
            param->setValueNotifyingHost (param->convertTo0to1 (value));
    };
    auto setChoice = [&] (const char* id, int index) {
        if (auto* param = dynamic_cast<juce::AudioParameterChoice*> (apvts.getParameter (id)))
            param->setValueNotifyingHost (param->convertTo0to1 ((float) index));
    };

    setChoice (kOsc1Wave, preset.osc1Wave);
    setChoice (kOsc2Wave, preset.osc2Wave);
    setFloat (kOscMix, preset.oscMix);
    setFloat (kSubLevel, preset.subLevel);
    setChoice (kSubWave, preset.subWave);
    setFloat (kCutoff, preset.cutoff);
    setFloat (kResonance, preset.resonance);
    setFloat (kFilterEnv, preset.filterEnvAmt);
    setFloat (kAmpA, preset.ampA);
    setFloat (kAmpD, preset.ampD);
    setFloat (kAmpS, preset.ampS);
    setFloat (kAmpR, preset.ampR);
    setFloat (kFiltA, preset.filtA);
    setFloat (kFiltD, preset.filtD);
    setFloat (kFiltS, preset.filtS);
    setFloat (kFiltR, preset.filtR);
    setChoice (kLfoTarget, preset.lfoTarget);
    setFloat (kLfoRate, preset.lfoRate);
    setFloat (kLfoDepth, preset.lfoDepth);
    setFloat (kGlide, preset.glide);
    setFloat (kDrive, preset.drive);
    setFloat (kChorus, preset.chorus);
    setFloat (kMasterGain, preset.masterGain);

    currentPresetName = preset.name;
}

bool NoctisProcessor::isFavorite (const juce::String& presetName) const
{
    return favoriteNames.contains (presetName);
}

void NoctisProcessor::toggleFavorite (const juce::String& presetName)
{
    if (isFavorite (presetName))
        favoriteNames.removeString (presetName);
    else
        favoriteNames.add (presetName);
}

//==============================================================================
const juce::String NoctisProcessor::getName() const { return JucePlugin_Name; }
bool NoctisProcessor::acceptsMidi() const { return true; }
bool NoctisProcessor::producesMidi() const { return false; }
bool NoctisProcessor::isMidiEffect() const { return false; }
double NoctisProcessor::getTailLengthSeconds() const { return 2.0; }

int NoctisProcessor::getNumPrograms() { return 1; }
int NoctisProcessor::getCurrentProgram() { return 0; }
void NoctisProcessor::setCurrentProgram (int) {}
const juce::String NoctisProcessor::getProgramName (int) { return currentPresetName; }
void NoctisProcessor::changeProgramName (int, const juce::String&) {}

void NoctisProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    // persist favorites + current preset name
    state.setProperty ("noctisFavorites", favoriteNames.joinIntoString ("|"), nullptr);
    state.setProperty ("noctisPresetName", currentPresetName, nullptr);
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void NoctisProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
    {
        auto state = juce::ValueTree::fromXml (*xml);
        if (state.isValid())
        {
            apvts.replaceState (state);
            favoriteNames = juce::StringArray::fromTokens (
                state.getProperty ("noctisFavorites", "").toString(), "|", "");
            currentPresetName = state.getProperty ("noctisPresetName", "Init").toString();
        }
    }
}

//==============================================================================
juce::AudioProcessorEditor* NoctisProcessor::createEditor()
{
    return new NoctisEditor (*this);
}

bool NoctisProcessor::hasEditor() const { return true; }

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new NoctisProcessor();
}
