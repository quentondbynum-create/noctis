#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"
#include "PresetRandomizer.h"

//==============================================================================
// NOCTIS color story: dark hardware, deep purple + electric blue glow.
// (Deliberately NOT amber — that's GODLIKE MOD's color.)
namespace NoctisColors
{
    const juce::Colour chassisDark   (0xff0b0b10);
    const juce::Colour chassisLight  (0xff17171f);
    const juce::Colour panelDark     (0xff101016);
    const juce::Colour steelDark     (0xff23232e);
    const juce::Colour steelLight    (0xff3d3d4d);
    const juce::Colour textMain      (0xffe9e9f2);
    const juce::Colour textDim       (0xff8a8a99);
    const juce::Colour purple        (0xff8b2ff7);
    const juce::Colour purpleGlow    (0xffb366ff);
    const juce::Colour blue          (0xff00d4ff);
    const juce::Colour blueGlow      (0xff66e5ff);
    const juce::Colour glassBlack    (0xff050508);
    const juce::Colour heartRed      (0xffff4d6d);
}

//==============================================================================
// Custom look & feel: chunky dark-metal knobs with purple/blue glow pointer.
class NoctisLookAndFeel : public juce::LookAndFeel_V4
{
public:
    NoctisLookAndFeel (NoctisProcessor& p) : processor (p) {}

    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                           juce::Slider& slider) override;

    void drawComboBox (juce::Graphics& g, int width, int height, bool isButtonDown,
                       int buttonX, int buttonY, int buttonW, int buttonH,
                       juce::ComboBox& box) override;

    void drawButtonBackground (juce::Graphics& g, juce::Button& button,
                               const juce::Colour& backgroundColour,
                               bool shouldDrawButtonAsHighlighted,
                               bool shouldDrawButtonAsDown) override;

    juce::Font getComboBoxFont (juce::ComboBox&) override;
    juce::Font getLabelFont (juce::Label&) override;

private:
    NoctisProcessor& processor;
};

//==============================================================================
// Hero XY performance pad: X = filter cutoff (log), Y = resonance.
// Black glass with animated purple/blue trail.
class NoctisXYPAD : public juce::Component,
                  private juce::AudioProcessorValueTreeState::Listener,
                  private juce::Timer
{
public:
    NoctisXYPAD (NoctisProcessor& proc);
    ~NoctisXYPAD() override;

    void paint (juce::Graphics& g) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent& e) override;
    void mouseDrag (const juce::MouseEvent& e) override;

private:
    void parameterChanged (const juce::String& paramID, float newValue) override;
    void timerCallback() override;
    void updateDotFromParams();
    void pushParamsFromDot (float nx, float ny); // normalized 0..1

    NoctisProcessor& processor;
    float dotX = 0.7f, dotY = 0.3f; // normalized
    struct TrailPoint { float x, y, age; };
    std::vector<TrailPoint> trail;
    bool dragging = false;
};

//==============================================================================
// VU-style output meters (stereo L/R) with purple/blue needle glow.
class NoctisVUMeter : public juce::Component, private juce::Timer
{
public:
    NoctisVUMeter (NoctisProcessor& proc);
    void paint (juce::Graphics& g) override;
    void resized() override;
private:
    void timerCallback() override;
    void drawSingleMeter (juce::Graphics& g, juce::Rectangle<float> bounds,
                          float level, float peak, const juce::String& label,
                          const juce::Colour& glow);
    NoctisProcessor& processor;
    float levelL = 0.0f, levelR = 0.0f;
    float peakL = 0.0f, peakR = 0.0f;
    int peakCountL = 0, peakCountR = 0;
};

//==============================================================================
// Playable piano keyboard — dark hardware styling (NOT default JUCE look).
// 3 octaves C2–B4, mouse velocity from click height, highlights DAW MIDI too.
class NoctisKeyboard : public juce::Component, private juce::Timer
{
public:
    NoctisKeyboard (NoctisProcessor& proc);
    ~NoctisKeyboard() override;

    void paint (juce::Graphics& g) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent& e) override;
    void mouseDrag (const juce::MouseEvent& e) override;
    void mouseUp (const juce::MouseEvent& e) override;

    static constexpr int kLoNote = 36; // C2
    static constexpr int kHiNote = 71; // B4

private:
    void timerCallback() override;
    int noteAtPosition (juce::Point<int> pos) const;
    bool isBlackKey (int note) const;
    int whiteKeyIndex (int note) const; // 0-based among white keys in range
    float velocityAt (int y) const;

    void noteOnMouse (int note, float velocity);
    void noteOffMouse (int note);

    NoctisProcessor& processor;
    bool dawNotes[128] = {};
    bool mouseNotes[128] = {};
    int heldMouseNote = -1;
};

//==============================================================================
class NoctisEditor : public juce::AudioProcessorEditor,
                   private juce::Timer,
                   private juce::AudioProcessorValueTreeState::Listener
{
public:
    NoctisEditor (NoctisProcessor&);
    ~NoctisEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void parameterChanged (const juce::String& paramID, float newValue) override;

    void refreshPresetBox();
    void loadPresetByIndex (int index);
    void updateHeart();
    void markModified();

    // user presets: ~/Documents/Headroom Audio/Noctis/Presets/*.noctis
    juce::File getPresetDir();
    void scanUserPresets();
    void saveUserPreset (const juce::String& name);
    NoctisPreset presetFromFile (const juce::File& f);
    NoctisPreset currentStateAsPreset (const juce::String& name);

    NoctisProcessor& processor;
    NoctisLookAndFeel lnf;

    // header
    juce::ComboBox presetBox;
    juce::TextButton heartButton { "♥" };
    juce::TextButton prevButton { "<" };
    juce::TextButton nextButton { ">" };
    juce::TextButton saveButton { "SAVE" };
    juce::TextButton diceButton { "DICE" };
    juce::Image logoImage;

    // hero
    NoctisXYPAD xyPad;
    NoctisVUMeter vuMeter;
    NoctisKeyboard keyboard;

    // wave selectors
    juce::ComboBox osc1WaveBox, osc2WaveBox;
    juce::Label osc1Label, osc2Label;

    // knobs (labels drawn underneath by paired labels)
    struct KnobUnit
    {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };
    std::vector<std::unique_ptr<KnobUnit>> knobs;

    void addKnob (const juce::String& paramID, const juce::String& labelText);

    std::vector<NoctisPreset> factoryPresets;
    std::vector<NoctisPreset> userPresets;
    std::vector<NoctisPreset> dicePresets; // generated by DICE button (cap 20)
    int currentPresetIndex = -1; // index into combined list (factory, user, dice)

    void onDicePressed();
    NoctisRandomizer randomizer;

    const NoctisPreset* presetAtCombinedIndex (int idx) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NoctisEditor)
};
