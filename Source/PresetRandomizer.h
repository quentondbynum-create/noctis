#pragma once

#include <juce_core/juce_core.h>
#include <vector>
#include <set>
#include "Presets.h"

//==============================================================================
/*
    NOCTIS smart randomizer ("DICE" button).

    Generates musical presets by archetype — never silent, never broken.
    Modes:
      - FRESH (60%): pick a random category archetype, randomize within its
        musical ranges (808s stay 808-ish, bells stay bell-ish, etc.)
      - MUTATE (25%): take the current preset, drift ~30% of params slightly
      - CROSSBREED (15%): interpolate two random factory presets 50/50 or 60/40

    Tracks the last 50 generated hashes; duplicates are regenerated.
*/
class NoctisRandomizer
{
public:
    NoctisRandomizer() = default;

    // Full randomize entry point. currentState is used for MUTATE mode;
    // pass an empty/default preset if none.
    NoctisPreset generate (const NoctisPreset& currentState,
                           const std::vector<NoctisPreset>& factory)
    {
        for (int attempt = 0; attempt < 12; ++attempt)
        {
            NoctisPreset p;
            const float roll = random.nextFloat();
            juce::String mode;

            if (roll < 0.25f && ! factory.empty())
            {
                p = mutate (currentState.name.isNotEmpty() ? currentState : factory[(size_t) random.nextInt ((int) factory.size())]);
                mode = "Mutate";
            }
            else if (roll < 0.40f && factory.size() >= 2)
            {
                const auto& a = factory[(size_t) random.nextInt ((int) factory.size())];
                const auto& b = factory[(size_t) random.nextInt ((int) factory.size())];
                p = crossbreed (a, b);
                mode = "Crossbreed";
            }
            else
            {
                p = freshArchetype();
                mode = "Fresh";
            }

            validate (p);
            const auto h = hashOf (p);
            if (history.find (h) != history.end())
                continue; // duplicate — roll again

            history.insert (h);
            if (history.size() > 50)
                history.erase (history.begin());

            ++diceCounter;
            p.name = "Dice: " + p.category + " #" + juce::String (diceCounter);
            p.factory = false;
            juce::ignoreUnused (mode);
            return p;
        }

        // fallback (shouldn't happen): fresh archetype, unvalidated-name anyway
        auto p = freshArchetype();
        validate (p);
        ++diceCounter;
        p.name = "Dice: " + p.category + " #" + juce::String (diceCounter);
        p.factory = false;
        return p;
    }

    int getDiceCount() const { return diceCounter; }

private:
    juce::Random random { juce::Random::getSystemRandom().nextInt() };
    std::set<juce::String> history;
    int diceCounter = 0;

    // ---- helpers ----
    float rf (float lo, float hi) { return lo + random.nextFloat() * (hi - lo); }
    int ri (int lo, int hi)       { return lo + random.nextInt (hi - lo + 1); }
    bool chance (float p)         { return random.nextFloat() < p; }

    static juce::String hashOf (const NoctisPreset& p)
    {
        // coarse hash — rounds floats so near-identical presets collide
        juce::String s;
        s << p.osc1Wave << "," << p.osc2Wave << "," << (int) p.oscMix << ","
          << (int) p.subLevel << "," << p.subWave << "," << (int) (p.cutoff / 50.0f) << ","
          << (int) p.resonance << "," << (int) p.filterEnvAmt << ","
          << (int) p.ampA << "," << (int) p.ampD << "," << (int) p.ampS << ","
          << (int) p.ampR << "," << (int) p.filtA << "," << (int) p.filtD << ","
          << (int) p.filtS << "," << (int) p.filtR << "," << p.lfoTarget << ","
          << (int) (p.lfoRate * 10.0f) << "," << (int) p.lfoDepth << ","
          << (int) p.glide << "," << (int) p.drive << "," << (int) p.chorus;
        return s;
    }

    // ---- archetype generators ----
    NoctisPreset freshArchetype()
    {
        static const char* cats[] = { "808", "Bells", "Plucks", "Pads", "Keys", "Leads", "Bass", "Weird" };
        const juce::String cat = cats[ri (0, 7)];
        NoctisPreset p;
        p.category = cat;

        if (cat == "808")      gen808 (p);
        else if (cat == "Bells") genBells (p);
        else if (cat == "Plucks") genPlucks (p);
        else if (cat == "Pads")   genPads (p);
        else if (cat == "Keys")   genKeys (p);
        else if (cat == "Leads")  genLeads (p);
        else if (cat == "Bass")   genBass (p);
        else                      genWeird (p);

        p.masterGain = rf (72.0f, 86.0f);
        return p;
    }

    void gen808 (NoctisPreset& p)
    {
        p.osc1Wave = 4;
        if (chance (0.35f)) { p.osc2Wave = 0; p.oscMix = rf (20.0f, 40.0f); }
        p.subLevel = rf (60.0f, 92.0f);
        p.cutoff = rf (500.0f, 2600.0f);
        p.resonance = rf (3.0f, 15.0f);
        p.filterEnvAmt = rf (-15.0f, 30.0f);
        p.ampA = rf (1.0f, 6.0f); p.ampD = rf (60.0f, 160.0f);
        p.ampS = rf (70.0f, 96.0f); p.ampR = rf (200.0f, 700.0f);
        p.glide = chance (0.7f) ? rf (20.0f, 250.0f) : 0.0f;
        p.drive = rf (4.0f, 60.0f);
        p.chorus = rf (0.0f, 12.0f);
        if (chance (0.25f)) { p.lfoTarget = 2; p.lfoRate = rf (3.0f, 7.0f); p.lfoDepth = rf (25.0f, 55.0f); }
    }

    void genBells (NoctisPreset& p)
    {
        p.osc1Wave = chance (0.8f) ? 5 : 3;
        if (chance (0.3f)) { p.osc2Wave = 0; p.oscMix = rf (25.0f, 45.0f); }
        p.cutoff = rf (2000.0f, 11000.0f);
        p.resonance = rf (3.0f, 22.0f);
        p.filterEnvAmt = rf (-40.0f, 10.0f);
        p.ampA = rf (1.0f, 10.0f); p.ampD = rf (500.0f, 2500.0f);
        p.ampS = rf (4.0f, 25.0f); p.ampR = rf (500.0f, 2600.0f);
        p.chorus = rf (10.0f, 45.0f);
        p.drive = rf (0.0f, 14.0f);
    }

    void genPlucks (NoctisPreset& p)
    {
        const int w = ri (0, 2);
        p.osc1Wave = (w == 0) ? 7 : (w == 1 ? 1 : 3);
        if (chance (0.3f)) { p.osc2Wave = 7; p.oscMix = rf (30.0f, 55.0f); }
        p.cutoff = rf (900.0f, 9000.0f);
        p.resonance = rf (5.0f, 25.0f);
        p.filterEnvAmt = rf (25.0f, 70.0f);
        p.ampA = rf (1.0f, 5.0f); p.ampD = rf (90.0f, 320.0f);
        p.ampS = rf (5.0f, 35.0f); p.ampR = rf (90.0f, 320.0f);
        p.filtA = rf (1.0f, 5.0f); p.filtD = rf (80.0f, 280.0f);
        p.filtS = rf (5.0f, 30.0f); p.filtR = p.ampR;
        p.drive = rf (4.0f, 30.0f);
        p.chorus = rf (0.0f, 22.0f);
    }

    void genPads (NoctisPreset& p)
    {
        const int w = ri (0, 3);
        p.osc1Wave = (w == 0) ? 1 : (w == 1 ? 6 : (w == 2 ? 0 : 3));
        if (chance (0.5f)) { p.osc2Wave = p.osc1Wave; p.oscMix = rf (35.0f, 60.0f); }
        if (chance (0.3f)) p.subLevel = rf (20.0f, 55.0f);
        p.cutoff = rf (700.0f, 3000.0f);
        p.resonance = rf (4.0f, 18.0f);
        p.filterEnvAmt = rf (-15.0f, 25.0f);
        p.ampA = rf (800.0f, 2500.0f); p.ampD = rf (500.0f, 1600.0f);
        p.ampS = rf (80.0f, 95.0f); p.ampR = rf (1500.0f, 3500.0f);
        p.filtA = p.ampA; p.filtD = p.ampD; p.filtS = rf (60.0f, 85.0f); p.filtR = p.ampR;
        p.chorus = rf (35.0f, 65.0f);
        p.drive = rf (3.0f, 14.0f);
        if (chance (0.45f)) { p.lfoTarget = 2; p.lfoRate = rf (0.15f, 1.0f); p.lfoDepth = rf (15.0f, 45.0f); }
    }

    void genKeys (NoctisPreset& p)
    {
        p.osc1Wave = chance (0.6f) ? 0 : 3;
        if (chance (0.4f)) { p.osc2Wave = 3; p.oscMix = rf (25.0f, 45.0f); }
        p.cutoff = rf (1400.0f, 5500.0f);
        p.resonance = rf (4.0f, 14.0f);
        p.filterEnvAmt = rf (0.0f, 30.0f);
        p.ampA = rf (1.0f, 20.0f); p.ampD = rf (350.0f, 900.0f);
        p.ampS = rf (35.0f, 65.0f); p.ampR = rf (280.0f, 900.0f);
        p.chorus = rf (15.0f, 42.0f);
        p.drive = rf (4.0f, 20.0f);
    }

    void genLeads (NoctisPreset& p)
    {
        const int w = ri (0, 2);
        p.osc1Wave = (w == 0) ? 1 : (w == 1 ? 2 : 0);
        if (chance (0.45f)) { p.osc2Wave = p.osc1Wave; p.oscMix = rf (35.0f, 60.0f); }
        if (chance (0.2f)) p.subLevel = rf (15.0f, 35.0f);
        p.cutoff = rf (2200.0f, 9500.0f);
        p.resonance = rf (6.0f, 35.0f);
        p.filterEnvAmt = rf (0.0f, 45.0f);
        p.ampA = rf (2.0f, 15.0f); p.ampD = rf (50.0f, 150.0f);
        p.ampS = rf (80.0f, 95.0f); p.ampR = rf (120.0f, 300.0f);
        p.glide = chance (0.5f) ? rf (15.0f, 200.0f) : 0.0f;
        p.drive = rf (8.0f, 60.0f);
        p.chorus = rf (5.0f, 30.0f);
    }

    void genBass (NoctisPreset& p)
    {
        p.osc1Wave = chance (0.6f) ? 1 : 2;
        if (chance (0.35f)) { p.osc2Wave = 0; p.oscMix = rf (30.0f, 50.0f); }
        p.subLevel = rf (40.0f, 85.0f);
        p.cutoff = rf (400.0f, 2600.0f);
        p.resonance = rf (4.0f, 25.0f);
        p.filterEnvAmt = rf (-10.0f, 50.0f);
        p.ampA = rf (2.0f, 8.0f); p.ampD = rf (70.0f, 200.0f);
        p.ampS = rf (70.0f, 95.0f); p.ampR = rf (140.0f, 400.0f);
        p.filtA = p.ampA; p.filtD = rf (70.0f, 180.0f);
        p.filtS = rf (35.0f, 65.0f); p.filtR = p.ampR;
        p.drive = rf (8.0f, 55.0f);
        p.chorus = rf (0.0f, 10.0f);
        if (chance (0.2f)) { p.lfoTarget = 2; p.lfoRate = rf (4.0f, 8.0f); p.lfoDepth = rf (40.0f, 70.0f); }
    }

    void genWeird (NoctisPreset& p)
    {
        p.osc1Wave = ri (0, 7);
        if (chance (0.5f)) { p.osc2Wave = ri (0, 7); p.oscMix = rf (25.0f, 65.0f); }
        if (chance (0.35f)) p.subLevel = rf (20.0f, 60.0f);
        p.cutoff = rf (700.0f, 8000.0f);
        p.resonance = rf (15.0f, 55.0f);
        p.filterEnvAmt = rf (-40.0f, 40.0f);
        p.ampA = rf (1.0f, 2000.0f); p.ampD = rf (60.0f, 1500.0f);
        p.ampS = rf (20.0f, 100.0f); p.ampR = rf (80.0f, 3500.0f);
        const int lt = ri (0, 3);
        p.lfoTarget = lt;
        if (lt != 0) { p.lfoRate = rf (0.1f, 14.0f); p.lfoDepth = rf (30.0f, 85.0f); }
        p.glide = chance (0.3f) ? rf (50.0f, 350.0f) : 0.0f;
        p.drive = rf (10.0f, 70.0f);
        p.chorus = rf (0.0f, 55.0f);
    }

    // ---- MUTATE: drift ~30% of params slightly from a base preset ----
    NoctisPreset mutate (const NoctisPreset& base)
    {
        NoctisPreset p = base;
        p.category = base.category.isNotEmpty() ? base.category : "Weird";

        auto mutF = [&] (float& v, float lo, float hi, float amt)
        {
            if (chance (0.3f))
                v = juce::jlimit (lo, hi, v + rf (-amt, amt));
        };
        auto mutI = [&] (int& v, int lo, int hi)
        {
            if (chance (0.3f))
                v = juce::jlimit (lo, hi, v + ri (-1, 1));
        };

        mutI (p.osc1Wave, 0, 7);
        mutI (p.osc2Wave, 0, 7);
        mutF (p.oscMix, 0.0f, 100.0f, 25.0f);
        mutF (p.subLevel, 0.0f, 100.0f, 25.0f);
        // cutoff in log-ish space: multiply/divide
        if (chance (0.3f))
            p.cutoff = juce::jlimit (40.0f, 18000.0f, p.cutoff * rf (0.6f, 1.7f));
        mutF (p.resonance, 0.0f, 80.0f, 18.0f);
        mutF (p.filterEnvAmt, -100.0f, 100.0f, 30.0f);
        mutF (p.ampA, 1.0f, 2000.0f, p.ampA * 0.8f + 2.0f);
        mutF (p.ampD, 1.0f, 2000.0f, p.ampD * 0.8f + 10.0f);
        mutF (p.ampS, 0.0f, 100.0f, 25.0f);
        mutF (p.ampR, 10.0f, 5000.0f, p.ampR * 0.8f + 20.0f);
        mutF (p.filtD, 1.0f, 2000.0f, p.filtD * 0.8f + 10.0f);
        mutF (p.filtS, 0.0f, 100.0f, 25.0f);
        if (chance (0.3f)) p.lfoTarget = ri (0, 3);
        if (p.lfoTarget != 0)
        {
            mutF (p.lfoRate, 0.05f, 20.0f, p.lfoRate * 0.7f + 0.2f);
            mutF (p.lfoDepth, 0.0f, 100.0f, 25.0f);
        }
        mutF (p.glide, 0.0f, 500.0f, 60.0f);
        mutF (p.drive, 0.0f, 100.0f, 20.0f);
        mutF (p.chorus, 0.0f, 100.0f, 20.0f);
        mutF (p.masterGain, 60.0f, 92.0f, 8.0f);
        return p;
    }

    // ---- CROSSBREED: interpolate two presets ----
    NoctisPreset crossbreed (const NoctisPreset& a, const NoctisPreset& b)
    {
        const float t = chance (0.5f) ? 0.5f : 0.6f; // 50/50 or 60/40
        NoctisPreset p;
        p.category = chance (0.5f) ? a.category : b.category;

        auto mixI = [&] (int av, int bv) { return (int) std::round (av * t + bv * (1.0f - t)); };
        auto mixF = [&] (float av, float bv) { return av * t + bv * (1.0f - t); };

        p.osc1Wave = chance (0.5f) ? a.osc1Wave : b.osc1Wave;
        p.osc2Wave = chance (0.5f) ? a.osc2Wave : b.osc2Wave;
        p.subWave  = chance (0.5f) ? a.subWave : b.subWave;
        p.oscMix = mixF (a.oscMix, b.oscMix);
        p.subLevel = mixF (a.subLevel, b.subLevel);
        // cutoff: geometric mix (log domain)
        p.cutoff = std::exp (mixF (std::log (juce::jmax (20.0f, a.cutoff)),
                                   std::log (juce::jmax (20.0f, b.cutoff))));
        p.resonance = mixF (a.resonance, b.resonance);
        p.filterEnvAmt = mixF (a.filterEnvAmt, b.filterEnvAmt);
        p.ampA = mixF (a.ampA, b.ampA); p.ampD = mixF (a.ampD, b.ampD);
        p.ampS = mixF (a.ampS, b.ampS); p.ampR = mixF (a.ampR, b.ampR);
        p.filtA = mixF (a.filtA, b.filtA); p.filtD = mixF (a.filtD, b.filtD);
        p.filtS = mixF (a.filtS, b.filtS); p.filtR = mixF (a.filtR, b.filtR);
        p.lfoTarget = chance (0.5f) ? a.lfoTarget : b.lfoTarget;
        p.lfoRate = mixF (a.lfoRate, b.lfoRate);
        p.lfoDepth = mixF (a.lfoDepth, b.lfoDepth);
        p.glide = mixF (a.glide, b.glide);
        p.drive = mixF (a.drive, b.drive);
        p.chorus = mixF (a.chorus, b.chorus);
        p.masterGain = mixF (a.masterGain, b.masterGain);
        juce::ignoreUnused (mixI);
        return p;
    }

    // ---- validation: never silent, never squealing ----
    void validate (NoctisPreset& p)
    {
        // waves in range
        p.osc1Wave = juce::jlimit (0, 7, p.osc1Wave);
        p.osc2Wave = juce::jlimit (0, 7, p.osc2Wave);
        p.subWave  = juce::jlimit (0, 1, p.subWave);
        p.lfoTarget = juce::jlimit (0, 3, p.lfoTarget);

        // cutoff must let something through: floor at 60 Hz unless it's
        // meant to be dark (then keep >= 120)
        p.cutoff = juce::jlimit (60.0f, 18000.0f, p.cutoff);

        // high resonance + nearly-closed filter = squeal: open up or calm down
        if (p.cutoff < 300.0f && p.resonance > 55.0f)
        {
            if (chance (0.5f)) p.cutoff = 300.0f;
            else p.resonance = 55.0f;
        }
        p.resonance = juce::jlimit (0.0f, 85.0f, p.resonance);

        // audible: keep master in a sane band
        p.masterGain = juce::jlimit (62.0f, 92.0f, p.masterGain);

        // envelopes sane
        p.ampA = juce::jlimit (1.0f, 2000.0f, p.ampA);
        p.ampD = juce::jlimit (1.0f, 2000.0f, p.ampD);
        p.ampS = juce::jlimit (0.0f, 100.0f, p.ampS);
        p.ampR = juce::jlimit (10.0f, 5000.0f, p.ampR);
        p.filtA = juce::jlimit (1.0f, 2000.0f, p.filtA);
        p.filtD = juce::jlimit (1.0f, 2000.0f, p.filtD);
        p.filtS = juce::jlimit (0.0f, 100.0f, p.filtS);
        p.filtR = juce::jlimit (10.0f, 5000.0f, p.filtR);

        p.oscMix = juce::jlimit (0.0f, 100.0f, p.oscMix);
        p.subLevel = juce::jlimit (0.0f, 100.0f, p.subLevel);
        p.filterEnvAmt = juce::jlimit (-100.0f, 100.0f, p.filterEnvAmt);
        p.lfoRate = juce::jlimit (0.05f, 20.0f, p.lfoRate);
        p.lfoDepth = juce::jlimit (0.0f, 100.0f, p.lfoDepth);
        p.glide = juce::jlimit (0.0f, 500.0f, p.glide);
        p.drive = juce::jlimit (0.0f, 100.0f, p.drive);
        p.chorus = juce::jlimit (0.0f, 100.0f, p.chorus);

        if (p.category.isEmpty())
            p.category = "Weird";
    }
};
