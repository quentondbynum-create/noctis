cmake_minimum_required(VERSION 3.22)
project(NOCTIS VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Pull JUCE automatically — first build downloads it (~150MB), then it's cached.
include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 7.0.12
)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(NOCTIS
    COMPANY_NAME "Headroom Audio"
    PLUGIN_NAME "NOCTIS"
    DESCRIPTION "NOCTIS — dark wavetable synth for Headroom Audio. 808s, bells, plucks, pads, keys and leads with a hardware soul."
    PLUGIN_MANUFACTURER_CODE Hdrm
    PLUGIN_CODE Noct
    BUNDLE_ID com.headroomaudio.noctis
    FORMATS AU VST3 Standalone
    PRODUCT_NAME "NOCTIS"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
)

target_sources(NOCTIS PRIVATE
    Source/WavetableSynth.h
    Source/WavetableSynth.cpp
    Source/Presets.h
    Source/PresetRandomizer.h
    Source/PluginProcessor.h
    Source/PluginProcessor.cpp
    Source/PluginEditor.h
    Source/PluginEditor.cpp
)

target_compile_definitions(NOCTIS PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

# Headroom Audio logo embedded as binary data (small brand mark in the UI)
juce_add_binary_data(NOCTISAssets SOURCES Source/Assets/headroom-logo.png)

target_link_libraries(NOCTIS PRIVATE
    juce::juce_audio_utils
    juce::juce_audio_processors
    juce::juce_audio_formats
    juce::juce_dsp
    juce::juce_graphics
    NOCTISAssets
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
)
