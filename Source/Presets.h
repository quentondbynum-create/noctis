#pragma once

#include <juce_core/juce_core.h>
#include <vector>

//==============================================================================
/*
    NOCTIS factory presets — 24 sounds programmed for warmth and weight.
    Diva/Omnisphere character, not thin ROMpler buzz.

    Values are in APVTS raw units (see NoctisProcessor::createParameterLayout).
*/
struct NoctisPreset
{
    juce::String name;
    juce::String category;
    bool factory = true;

    int   osc1Wave = 1, osc2Wave = 1; // 0=sine 1=saw 2=square 3=tri 4=808 5=bell 6=ssaw 7=pluck
    float oscMix = 50.0f;             // 0..100
    float subLevel = 0.0f;            // 0..100
    int   subWave = 0;                // 0=sine 1=square
    float cutoff = 8000.0f;           // Hz
    float resonance = 10.0f;          // 0..100
    float filterEnvAmt = 0.0f;         // -100..100
    float ampA = 5.0f, ampD = 100.0f, ampS = 80.0f, ampR = 150.0f;   // ms, ms, %, ms
    float filtA = 5.0f, filtD = 100.0f, filtS = 50.0f, filtR = 150.0f;
    int   lfoTarget = 0;              // 0=off 1=pitch 2=filter 3=amp
    float lfoRate = 5.0f;             // Hz
    float lfoDepth = 0.0f;            // 0..100
    float glide = 0.0f;               // ms
    float drive = 0.0f;               // 0..100
    float chorus = 0.0f;              // 0..100
    float masterGain = 80.0f;         // 0..100
};

inline std::vector<NoctisPreset> getNoctisFactoryPresets()
{
    std::vector<NoctisPreset> p;
    NoctisPreset q;

    // ================= 808s =================
    q = NoctisPreset(); q.name = "Deep Sub 808"; q.category = "808";
    q.osc1Wave = 4; q.subLevel = 85; q.cutoff = 900; q.resonance = 5;
    q.ampA = 3; q.ampD = 80; q.ampS = 90; q.ampR = 400;
    q.glide = 60; q.drive = 10; q.masterGain = 85; p.push_back (q);

    q = NoctisPreset(); q.name = "Distorted 808"; q.category = "808";
    q.osc1Wave = 4; q.subLevel = 70; q.cutoff = 2500; q.resonance = 12;
    q.ampA = 2; q.ampD = 60; q.ampS = 85; q.ampR = 300;
    q.glide = 40; q.drive = 55; q.masterGain = 80; p.push_back (q);

    q = NoctisPreset(); q.name = "Short 808"; q.category = "808";
    q.osc1Wave = 4; q.subLevel = 75; q.cutoff = 1200;
    q.ampA = 2; q.ampD = 120; q.ampS = 20; q.ampR = 120;
    q.drive = 15; q.masterGain = 85; p.push_back (q);

    q = NoctisPreset(); q.name = "Glide 808"; q.category = "808";
    q.osc1Wave = 4; q.osc2Wave = 0; q.oscMix = 30; q.subLevel = 80;
    q.cutoff = 700; q.ampA = 4; q.ampD = 100; q.ampS = 90; q.ampR = 500;
    q.glide = 220; q.drive = 12; q.chorus = 10; q.masterGain = 85; p.push_back (q);

    // ================= Bells =================
    q = NoctisPreset(); q.name = "Dark Bell"; q.category = "Bells";
    q.osc1Wave = 5; q.cutoff = 4500; q.resonance = 8;
    q.ampA = 2; q.ampD = 900; q.ampS = 15; q.ampR = 900;
    q.filterEnvAmt = -25; q.drive = 5; q.chorus = 20; q.masterGain = 78; p.push_back (q);

    q = NoctisPreset(); q.name = "Glass Bell"; q.category = "Bells";
    q.osc1Wave = 5; q.osc2Wave = 0; q.oscMix = 40; q.cutoff = 9000; q.resonance = 5;
    q.ampA = 1; q.ampD = 1200; q.ampS = 10; q.ampR = 1200;
    q.chorus = 35; q.masterGain = 75; p.push_back (q);

    q = NoctisPreset(); q.name = "Music Box"; q.category = "Bells";
    q.osc1Wave = 3; q.osc2Wave = 0; q.oscMix = 50; q.cutoff = 6000;
    q.ampA = 1; q.ampD = 500; q.ampS = 5; q.ampR = 600;
    q.chorus = 15; q.masterGain = 78; p.push_back (q);

    q = NoctisPreset(); q.name = "Temple Bell"; q.category = "Bells";
    q.osc1Wave = 5; q.cutoff = 2200; q.resonance = 15;
    q.ampA = 8; q.ampD = 2000; q.ampS = 20; q.ampR = 2500;
    q.filterEnvAmt = -30; q.drive = 8; q.chorus = 25; q.masterGain = 80; p.push_back (q);

    // ================= Plucks =================
    q = NoctisPreset(); q.name = "Trap Pluck"; q.category = "Plucks";
    q.osc1Wave = 7; q.cutoff = 3200; q.resonance = 18; q.filterEnvAmt = 55;
    q.ampA = 2; q.ampD = 180; q.ampS = 25; q.ampR = 180;
    q.filtA = 2; q.filtD = 160; q.filtS = 20; q.filtR = 180;
    q.drive = 12; q.masterGain = 82; p.push_back (q);

    q = NoctisPreset(); q.name = "Soft Pluck"; q.category = "Plucks";
    q.osc1Wave = 3; q.osc2Wave = 7; q.oscMix = 35; q.cutoff = 2400; q.resonance = 8;
    q.filterEnvAmt = 35; q.ampA = 4; q.ampD = 220; q.ampS = 30; q.ampR = 220;
    q.filtA = 4; q.filtD = 200; q.filtS = 25;
    q.chorus = 20; q.masterGain = 80; p.push_back (q);

    q = NoctisPreset(); q.name = "Bright Pluck"; q.category = "Plucks";
    q.osc1Wave = 1; q.cutoff = 7500; q.resonance = 12; q.filterEnvAmt = 45;
    q.ampA = 1; q.ampD = 140; q.ampS = 20; q.ampR = 140;
    q.filtA = 1; q.filtD = 130; q.filtS = 15;
    q.drive = 8; q.masterGain = 80; p.push_back (q);

    q = NoctisPreset(); q.name = "Muted Pluck"; q.category = "Plucks";
    q.osc1Wave = 7; q.cutoff = 1100; q.resonance = 10; q.filterEnvAmt = 30;
    q.ampA = 2; q.ampD = 130; q.ampS = 15; q.ampR = 130;
    q.filtA = 2; q.filtD = 120; q.filtS = 10;
    q.drive = 18; q.masterGain = 82; p.push_back (q);

    // ================= Pads =================
    q = NoctisPreset(); q.name = "Dark Pad"; q.category = "Pads";
    q.osc1Wave = 1; q.osc2Wave = 1; q.oscMix = 50; q.cutoff = 1400; q.resonance = 12;
    q.ampA = 900; q.ampD = 800; q.ampS = 85; q.ampR = 1800;
    q.filterEnvAmt = 20; q.filtA = 900; q.filtD = 800; q.filtS = 70; q.filtR = 1800;
    q.chorus = 45; q.drive = 6; q.masterGain = 78; p.push_back (q);

    q = NoctisPreset(); q.name = "Lush Pad"; q.category = "Pads";
    q.osc1Wave = 6; q.cutoff = 2600; q.resonance = 8;
    q.ampA = 1200; q.ampD = 1000; q.ampS = 90; q.ampR = 2200;
    q.chorus = 60; q.drive = 4; q.masterGain = 76; p.push_back (q);

    q = NoctisPreset(); q.name = "Ambient Pad"; q.category = "Pads";
    q.osc1Wave = 0; q.osc2Wave = 3; q.oscMix = 50; q.cutoff = 1800;
    q.ampA = 2000; q.ampD = 1500; q.ampS = 90; q.ampR = 3000;
    q.lfoTarget = 2; q.lfoRate = 0.4f; q.lfoDepth = 30;
    q.chorus = 55; q.masterGain = 75; p.push_back (q);

    q = NoctisPreset(); q.name = "Choir-ish Pad"; q.category = "Pads";
    q.osc1Wave = 1; q.osc2Wave = 0; q.oscMix = 45; q.cutoff = 2000; q.resonance = 10;
    q.ampA = 1000; q.ampD = 900; q.ampS = 88; q.ampR = 2000;
    q.lfoTarget = 2; q.lfoRate = 0.6f; q.lfoDepth = 22;
    q.chorus = 50; q.drive = 5; q.masterGain = 77; p.push_back (q);

    // ================= Keys =================
    q = NoctisPreset(); q.name = "Rhodes-ish"; q.category = "Keys";
    q.osc1Wave = 0; q.osc2Wave = 3; q.oscMix = 40; q.cutoff = 2800; q.resonance = 6;
    q.ampA = 3; q.ampD = 600; q.ampS = 45; q.ampR = 400;
    q.chorus = 25; q.drive = 10; q.masterGain = 80; p.push_back (q);

    q = NoctisPreset(); q.name = "Electric Piano"; q.category = "Keys";
    q.osc1Wave = 3; q.cutoff = 3500;
    q.ampA = 2; q.ampD = 700; q.ampS = 40; q.ampR = 500;
    q.chorus = 20; q.drive = 8; q.masterGain = 80; p.push_back (q);

    q = NoctisPreset(); q.name = "Clav-ish"; q.category = "Keys";
    q.osc1Wave = 2; q.cutoff = 4200; q.resonance = 14; q.filterEnvAmt = 25;
    q.ampA = 1; q.ampD = 120; q.ampS = 35; q.ampR = 150;
    q.filtA = 1; q.filtD = 110; q.filtS = 30;
    q.drive = 20; q.masterGain = 80; p.push_back (q);

    q = NoctisPreset(); q.name = "Soft Keys"; q.category = "Keys";
    q.osc1Wave = 0; q.osc2Wave = 0; q.cutoff = 2200;
    q.ampA = 12; q.ampD = 500; q.ampS = 60; q.ampR = 600;
    q.chorus = 35; q.masterGain = 78; p.push_back (q);

    // ================= Leads =================
    q = NoctisPreset(); q.name = "Saw Lead"; q.category = "Leads";
    q.osc1Wave = 1; q.osc2Wave = 1; q.oscMix = 50; q.cutoff = 5200; q.resonance = 15;
    q.ampA = 4; q.ampD = 80; q.ampS = 85; q.ampR = 180;
    q.glide = 35; q.drive = 25; q.chorus = 15; q.masterGain = 80; p.push_back (q);

    q = NoctisPreset(); q.name = "Square Lead"; q.category = "Leads";
    q.osc1Wave = 2; q.cutoff = 4800; q.resonance = 12;
    q.ampA = 3; q.ampD = 70; q.ampS = 85; q.ampR = 160;
    q.glide = 25; q.drive = 20; q.chorus = 10; q.masterGain = 80; p.push_back (q);

    q = NoctisPreset(); q.name = "Soft Lead"; q.category = "Leads";
    q.osc1Wave = 0; q.osc2Wave = 1; q.oscMix = 55; q.subLevel = 25; q.cutoff = 3600;
    q.ampA = 10; q.ampD = 120; q.ampS = 85; q.ampR = 250;
    q.glide = 50; q.chorus = 30; q.drive = 8; q.masterGain = 80; p.push_back (q);

    q = NoctisPreset(); q.name = "Aggressive Lead"; q.category = "Leads";
    q.osc1Wave = 1; q.osc2Wave = 2; q.oscMix = 60; q.cutoff = 6800; q.resonance = 28;
    q.filterEnvAmt = 30; q.ampA = 2; q.ampD = 60; q.ampS = 90; q.ampR = 140;
    q.filtA = 2; q.filtD = 90; q.filtS = 60;
    q.drive = 45; q.masterGain = 78; p.push_back (q);

    return p;
}
