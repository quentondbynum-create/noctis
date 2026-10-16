#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <vector>
#include "WavetableSynth.h"
#include "Presets.h"

//==============================================================================
/*
    NOCTIS — dark wavetable synth for Headroom Audio.
    16-voice polyphonic, analog-character DSP (polyBLEP oscs, ladder filter,
    per-voice drift/variation, 2x oversampled drive).
*/
class NoctisProcessor : public juce::AudioProcessor
{
public:
    NoctisProcessor();
    ~NoctisProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // ---- parameters ----
    juce::AudioProcessorValueTreeState apvts;

    static constexpr const char* kOsc1Wave   = "osc1Wave";   // choice 0..7
    static constexpr const char* kOsc2Wave   = "osc2Wave";   // choice 0..7
    static constexpr const char* kOscMix     = "oscMix";     // %, 0..100
    static constexpr const char* kSubLevel   = "subLevel";   // %, 0..100
    static constexpr const char* kSubWave    = "subWave";    // choice 0..1
    static constexpr const char* kCutoff     = "cutoff";     // Hz, 20..20000
    static constexpr const char* kResonance  = "resonance";  // %, 0..100
    static constexpr const char* kFilterEnv  = "filterEnv";  // %, -100..100
    static constexpr const char* kAmpA       = "ampA";       // ms
    static constexpr const char* kAmpD       = "ampD";       // ms
    static constexpr const char* kAmpS       = "ampS";       // %
    static constexpr const char* kAmpR       = "ampR";       // ms
    static constexpr const char* kFiltA      = "filtA";      // ms
    static constexpr const char* kFiltD      = "filtD";      // ms
    static constexpr const char* kFiltS      = "filtS";      // %
    static constexpr const char* kFiltR      = "filtR";      // ms
    static constexpr const char* kLfoTarget  = "lfoTarget";  // choice 0..3
    static constexpr const char* kLfoRate    = "lfoRate";    // Hz
    static constexpr const char* kLfoDepth   = "lfoDepth";   // %
    static constexpr const char* kGlide      = "glide";      // ms
    static constexpr const char* kDrive      = "drive";      // %
    static constexpr const char* kChorus     = "chorus";     // %
    static constexpr const char* kMasterGain = "masterGain"; // %

    // Preset handling
    void applyPreset (const NoctisPreset& preset);
    juce::String getCurrentPresetName() const { return currentPresetName; }
    void setCurrentPresetName (const juce::String& n) { currentPresetName = n; }

    // Favorites (persisted in state)
    bool isFavorite (const juce::String& presetName) const;
    void toggleFavorite (const juce::String& presetName);

    // Peak for VU meters (stereo)
    float getPeakLevelL() const { return synth.getPeakLevelL(); }
    float getPeakLevelR() const { return synth.getPeakLevelR(); }
    float getPeakLevel() const { return synth.getPeakLevel(); }

    // On-screen keyboard -> synth: thread-safe MIDI injection from the UI.
    void postMidiMessage (const juce::MidiMessage& msg)
    {
        juce::ScopedLock sl (midiQueueLock);
        if (midiQueue.size() < 256)
            midiQueue.push_back (msg);
    }

    // Active notes for keyboard highlight.
    void getActiveNotes (bool notes[128]) const { synth.getActiveNotes (notes); }

    // Audio-reactive glow color (message thread safe).
    juce::Colour getGlowColour() const { return synth.getGlowColour(); }

private:
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    NoctisSynthEngine synth;
    juce::String currentPresetName = "Init";
    juce::StringArray favoriteNames;

    // Lock-free-ish MIDI queue from the on-screen keyboard (UI thread).
    juce::CriticalSection midiQueueLock;
    std::vector<juce::MidiMessage> midiQueue;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NoctisProcessor)
};
