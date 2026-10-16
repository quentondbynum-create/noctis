#include <BinaryData.h>
#include "PluginEditor.h"

//==============================================================================
// NoctisLookAndFeel
//==============================================================================
void NoctisLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                        float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                                        juce::Slider&)
{
    const float cx = x + width * 0.5f;
    const float cy = y + height * 0.5f;
    const float radius = juce::jmin (width, height) * 0.5f - 4.0f;

    // drop shadow
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.fillEllipse (cx - radius + 2, cy - radius + 4, radius * 2, radius * 2);

    // metal body: dark steel radial
    juce::ColourGradient metal (NoctisColors::steelLight, cx - radius * 0.4f, cy - radius * 0.4f,
                                NoctisColors::steelDark,  cx + radius * 0.5f, cy + radius * 0.5f, true);
    g.setGradientFill (metal);
    g.fillEllipse (cx - radius, cy - radius, radius * 2, radius * 2);

    // brushed ring highlight
    g.setColour (NoctisColors::steelLight.withAlpha (0.5f));
    g.drawEllipse (cx - radius + 1, cy - radius + 1, (radius - 1) * 2, (radius - 1) * 2, 1.5f);

    // knurled edge ticks
    g.setColour (juce::Colours::black.withAlpha (0.45f));
    for (int i = 0; i < 24; ++i)
    {
        const float a = i * juce::MathConstants<float>::twoPi / 24.0f;
        const float r1 = radius - 2.0f, r2 = radius - 5.0f;
        g.drawLine (cx + std::cos (a) * r1, cy + std::sin (a) * r1,
                    cx + std::cos (a) * r2, cy + std::sin (a) * r2, 1.2f);
    }

    // glow arc (value) — AUDIO-REACTIVE color
    const auto glow = processor.getGlowColour();
    const float angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
    juce::Path arc;
    arc.addCentredArc (cx, cy, radius - 8.0f, radius - 8.0f,
                       0.0f, rotaryStartAngle, angle, true);
    g.setColour (glow.withAlpha (0.85f));
    g.strokePath (arc, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved));

    // pointer line with reactive glow tip
    const float px = cx + std::cos (angle - juce::MathConstants<float>::halfPi) * (radius - 10.0f);
    const float py = cy + std::sin (angle - juce::MathConstants<float>::halfPi) * (radius - 10.0f);
    g.setColour (glow.brighter (0.35f));
    g.drawLine (cx, cy, px, py, 3.0f);
    g.setColour (glow.withAlpha (0.35f));
    g.drawLine (cx, cy, px, py, 7.0f);

    // center cap
    juce::ColourGradient cap (NoctisColors::chassisLight, cx, cy - 4,
                              NoctisColors::chassisDark, cx, cy + 6, false);
    g.setGradientFill (cap);
    g.fillEllipse (cx - 7, cy - 7, 14, 14);
}

void NoctisLookAndFeel::drawComboBox (juce::Graphics& g, int width, int height, bool,
                                    int, int, int, int, juce::ComboBox&)
{
    // dark glass combo
    juce::ColourGradient glass (NoctisColors::steelDark, 0, 0,
                                NoctisColors::panelDark, 0, (float) height, false);
    g.setGradientFill (glass);
    g.fillRoundedRectangle (0, 0, width, height, 6.0f);
    g.setColour (NoctisColors::purple.withAlpha (0.5f));
    g.drawRoundedRectangle (0.5f, 0.5f, width - 1, height - 1, 6.0f, 1.0f);

    // dropdown arrow
    juce::Path arrow;
    const float ax = width - 16.0f, ay = height * 0.5f;
    arrow.addTriangle (ax - 5, ay - 3, ax + 5, ay - 3, ax, ay + 4);
    g.setColour (NoctisColors::textDim);
    g.fillPath (arrow);
}

void NoctisLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& button,
                                            const juce::Colour&, bool highlighted, bool down)
{
    const auto bounds = button.getLocalBounds().toFloat();
    const bool isHeart = button.getName() == "heart";

    juce::Colour base = isHeart ? NoctisColors::panelDark : NoctisColors::steelDark;
    if (highlighted) base = base.brighter (0.12f);
    if (down)        base = base.darker (0.15f);

    juce::ColourGradient grad (base.brighter (0.08f), 0, 0, base.darker (0.1f), 0, bounds.getHeight(), false);
    g.setGradientFill (grad);
    g.fillRoundedRectangle (bounds, 6.0f);

    if (isHeart && button.getToggleState())
    {
        g.setColour (NoctisColors::heartRed.withAlpha (0.9f));
        g.drawRoundedRectangle (bounds.reduced (0.5f), 6.0f, 1.5f);
    }
    else
    {
        g.setColour (NoctisColors::purple.withAlpha (highlighted ? 0.7f : 0.35f));
        g.drawRoundedRectangle (bounds.reduced (0.5f), 6.0f, 1.0f);
    }
}

juce::Font NoctisLookAndFeel::getComboBoxFont (juce::ComboBox&)
{
    juce::Font f (14.0f, juce::Font::bold);
    f.setExtraKerningFactor (0.04f);
    return f;
}

juce::Font NoctisLookAndFeel::getLabelFont (juce::Label&)
{
    juce::Font f (11.0f, juce::Font::bold);
    f.setExtraKerningFactor (0.08f);
    return f;
}

//==============================================================================
// NoctisXYPAD
//==============================================================================
NoctisXYPAD::NoctisXYPAD (NoctisProcessor& proc) : processor (proc)
{
    processor.apvts.addParameterListener (NoctisProcessor::kCutoff, this);
    processor.apvts.addParameterListener (NoctisProcessor::kResonance, this);
    updateDotFromParams();
    startTimerHz (30);
}

NoctisXYPAD::~NoctisXYPAD()
{
    processor.apvts.removeParameterListener (NoctisProcessor::kCutoff, this);
    processor.apvts.removeParameterListener (NoctisProcessor::kResonance, this);
}

void NoctisXYPAD::parameterChanged (const juce::String&, float)
{
    // called on message thread via APVTS listener
    updateDotFromParams();
}

void NoctisXYPAD::updateDotFromParams()
{
    const float cutoff = processor.apvts.getRawParameterValue (NoctisProcessor::kCutoff)->load();
    const float reso   = processor.apvts.getRawParameterValue (NoctisProcessor::kResonance)->load();
    // log map 20..20000 -> 0..1
    dotX = juce::jlimit (0.0f, 1.0f,
            (std::log10 (cutoff) - std::log10 (20.0f)) / (std::log10 (20000.0f) - std::log10 (20.0f)));
    dotY = juce::jlimit (0.0f, 1.0f, 1.0f - reso / 100.0f); // top = max resonance
    repaint();
}

void NoctisXYPAD::pushParamsFromDot (float nx, float ny)
{
    nx = juce::jlimit (0.0f, 1.0f, nx);
    ny = juce::jlimit (0.0f, 1.0f, ny);
    const float cutoff = std::pow (10.0f, std::log10 (20.0f)
                        + nx * (std::log10 (20000.0f) - std::log10 (20.0f)));
    const float reso = (1.0f - ny) * 100.0f;

    if (auto* p = processor.apvts.getParameter (NoctisProcessor::kCutoff))
        p->setValueNotifyingHost (p->convertTo0to1 (cutoff));
    if (auto* p = processor.apvts.getParameter (NoctisProcessor::kResonance))
        p->setValueNotifyingHost (p->convertTo0to1 (reso));
}

void NoctisXYPAD::mouseDown (const juce::MouseEvent& e)
{
    dragging = true;
    const float nx = (float) e.x / getWidth();
    const float ny = (float) e.y / getHeight();
    dotX = nx; dotY = ny;
    trail.push_back ({ nx, ny, 1.0f });
    pushParamsFromDot (nx, ny);
}

void NoctisXYPAD::mouseDrag (const juce::MouseEvent& e)
{
    if (! dragging) return;
    const float nx = juce::jlimit (0.0f, 1.0f, (float) e.x / getWidth());
    const float ny = juce::jlimit (0.0f, 1.0f, (float) e.y / getHeight());
    dotX = nx; dotY = ny;
    trail.push_back ({ nx, ny, 1.0f });
    if (trail.size() > 60) trail.erase (trail.begin());
    pushParamsFromDot (nx, ny);
}

void NoctisXYPAD::timerCallback()
{
    bool changed = false;
    for (auto& t : trail) { t.age -= 0.06f; changed = true; }
    trail.erase (std::remove_if (trail.begin(), trail.end(),
                                 [] (const TrailPoint& t) { return t.age <= 0.0f; }),
                 trail.end());
    if (changed || dragging) repaint();
}

void NoctisXYPAD::resized() {}

void NoctisXYPAD::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();

    // black glass
    juce::ColourGradient glass (juce::Colour (0xff0d0d14), 0, 0,
                                NoctisColors::glassBlack, 0, bounds.getHeight(), false);
    g.setGradientFill (glass);
    g.fillRoundedRectangle (bounds, 10.0f);

    // subtle grid
    g.setColour (juce::Colours::white.withAlpha (0.045f));
    for (int i = 1; i < 4; ++i)
    {
        const float gx = bounds.getWidth() * i / 4.0f;
        const float gy = bounds.getHeight() * i / 4.0f;
        g.drawVerticalLine ((int) gx, 0, bounds.getHeight());
        g.drawHorizontalLine ((int) gy, 0, bounds.getWidth());
    }

    // trail: AUDIO-REACTIVE glowing ribbon
    const auto glow = processor.getGlowColour();
    const auto glowBright = glow.brighter (0.4f);
    for (size_t i = 1; i < trail.size(); ++i)
    {
        const auto& a = trail[i - 1];
        const auto& b = trail[i];
        const float alpha = juce::jmax (0.0f, b.age) * 0.75f;
        g.setColour (glow.withAlpha (alpha));
        g.drawLine (a.x * bounds.getWidth(), a.y * bounds.getHeight(),
                    b.x * bounds.getWidth(), b.y * bounds.getHeight(), 5.0f);
        g.setColour (glowBright.withAlpha (alpha * 0.9f));
        g.drawLine (a.x * bounds.getWidth(), a.y * bounds.getHeight(),
                    b.x * bounds.getWidth(), b.y * bounds.getHeight(), 2.0f);
    }

    // the dot: glowing orb (reactive)
    const float dx = dotX * bounds.getWidth();
    const float dy = dotY * bounds.getHeight();
    g.setColour (glow.withAlpha (0.30f));
    g.fillEllipse (dx - 22, dy - 22, 44, 44);
    g.setColour (glowBright.withAlpha (0.9f));
    g.fillEllipse (dx - 7, dy - 7, 14, 14);
    g.setColour (juce::Colours::white);
    g.fillEllipse (dx - 2.5f, dy - 2.5f, 5, 5);

    // glass border (reactive tint)
    g.setColour (glow.withAlpha (0.55f));
    g.drawRoundedRectangle (bounds.reduced (0.5f), 10.0f, 1.5f);

    g.setColour (NoctisColors::textDim);
    juce::Font __kf (10.0f, juce::Font::bold);
    __kf.setExtraKerningFactor (0.12f);
    g.setFont (__kf);
    g.drawText ("CUTOFF", 10, 6, 90, 14, juce::Justification::left);
    g.drawText ("RESO", bounds.getWidth() - 100, 6, 90, 14, juce::Justification::right);
}

//==============================================================================
// NoctisVUMeter — two vintage meters (L/R)
//==============================================================================
NoctisVUMeter::NoctisVUMeter (NoctisProcessor& proc) : processor (proc)
{
    startTimerHz (30);
}

void NoctisVUMeter::resized() {}

void NoctisVUMeter::timerCallback()
{
    const float tL = juce::jlimit (0.0f, 1.0f, processor.getPeakLevelL());
    const float tR = juce::jlimit (0.0f, 1.0f, processor.getPeakLevelR());
    levelL += (tL - levelL) * 0.45f;
    levelR += (tR - levelR) * 0.45f;
    if (tL >= peakL) { peakL = tL; peakCountL = 0; }
    else if (++peakCountL > 40) peakL = juce::jmax (0.0f, peakL - 0.02f);
    if (tR >= peakR) { peakR = tR; peakCountR = 0; }
    else if (++peakCountR > 40) peakR = juce::jmax (0.0f, peakR - 0.02f);
    repaint();
}

void NoctisVUMeter::drawSingleMeter (juce::Graphics& g, juce::Rectangle<float> bounds,
                                     float level, float peak, const juce::String& label,
                                     const juce::Colour& glow)
{
    g.setColour (NoctisColors::panelDark);
    g.fillRoundedRectangle (bounds, 8.0f);

    // AUDIO-REACTIVE backlight wash behind the meter
    juce::ColourGradient wash (glow.withAlpha (0.16f), bounds.getCentreX(), bounds.getY(),
                               juce::Colours::transparentBlack, bounds.getCentreX(), bounds.getBottom(), false);
    g.setGradientFill (wash);
    g.fillRoundedRectangle (bounds, 8.0f);

    const float cx = bounds.getCentreX();
    const float cy = bounds.getBottom() - 6.0f;
    const float r = juce::jmin (bounds.getWidth(), bounds.getHeight() * 1.7f) * 0.40f;

    const float a0 = -juce::MathConstants<float>::pi * 0.78f;
    const float a1 = -juce::MathConstants<float>::pi * 0.22f;

    g.setColour (NoctisColors::textDim.withAlpha (0.6f));
    juce::Path arc;
    arc.addCentredArc (cx, cy, r, r, 0.0f, a0, a1, true);
    g.strokePath (arc, juce::PathStrokeType (1.5f));

    g.setFont (juce::Font (8.0f));
    g.setColour (NoctisColors::textDim);
    const char* marks[] = { "-24", "-12", "-6", "0" };
    for (int i = 0; i < 4; ++i)
    {
        const float a = a0 + (a1 - a0) * i / 3.0f;
        const float lx = cx + std::cos (a) * (r - 12.0f);
        const float ly = cy + std::sin (a) * (r - 12.0f);
        g.drawText (marks[i], (int) lx - 12, (int) ly - 7, 24, 14, juce::Justification::centred);
    }

    const float lvl = std::sqrt (juce::jlimit (0.0f, 1.0f, level));
    const float na = a0 + (a1 - a0) * lvl;
    g.setColour (glow.withAlpha (0.35f));
    g.drawLine (cx, cy, cx + std::cos (na) * r, cy + std::sin (na) * r, 5.0f);
    g.setColour (glow.brighter (0.3f));
    g.drawLine (cx, cy, cx + std::cos (na) * r, cy + std::sin (na) * r, 2.0f);

    const float pa = a0 + (a1 - a0) * std::sqrt (juce::jlimit (0.0f, 1.0f, peak));
    g.setColour (glow);
    g.drawLine (cx + std::cos (pa) * (r - 5), cy + std::sin (pa) * (r - 5),
                cx + std::cos (pa) * r,       cy + std::sin (pa) * r, 2.5f);

    g.setColour (NoctisColors::steelDark);
    g.fillEllipse (cx - 5, cy - 5, 10, 10);

    g.setColour (NoctisColors::textDim);
    juce::Font __kf (10.0f, juce::Font::bold);
    __kf.setExtraKerningFactor (0.14f);
    g.setFont (__kf);
    g.drawText (label, bounds.getX(), bounds.getY() + 3, bounds.getWidth(), 13,
                juce::Justification::centred);
}

void NoctisVUMeter::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    const auto glow = processor.getGlowColour();
    const float halfW = bounds.getWidth() * 0.5f;
    drawSingleMeter (g, bounds.withWidth (halfW - 3), levelL, peakL, "LEFT", glow);
    drawSingleMeter (g, bounds.withX (bounds.getX() + halfW + 3).withWidth (halfW - 3),
                     levelR, peakR, "RIGHT", glow);
}

//==============================================================================
// NoctisKeyboard — dark hardware piano, C2–B4
//==============================================================================
NoctisKeyboard::NoctisKeyboard (NoctisProcessor& proc) : processor (proc)
{
    startTimerHz (30);
}

NoctisKeyboard::~NoctisKeyboard()
{
    // release any mouse-held notes
    for (int n = kLoNote; n <= kHiNote; ++n)
        if (mouseNotes[n])
            processor.postMidiMessage (juce::MidiMessage::noteOff (1, n));
}

void NoctisKeyboard::resized() {}

bool NoctisKeyboard::isBlackKey (int note) const
{
    const int pc = note % 12;
    return pc == 1 || pc == 3 || pc == 6 || pc == 8 || pc == 10;
}

int NoctisKeyboard::whiteKeyIndex (int note) const
{
    int idx = 0;
    for (int n = kLoNote; n < note; ++n)
        if (! isBlackKey (n)) ++idx;
    return idx;
}

int NoctisKeyboard::noteAtPosition (juce::Point<int> pos) const
{
    const int W = getWidth(), H = getHeight();
    if (W <= 0 || H <= 0) return -1;

    // count white keys in range
    int numWhite = 0;
    for (int n = kLoNote; n <= kHiNote; ++n)
        if (! isBlackKey (n)) ++numWhite;
    const float whiteW = (float) W / numWhite;
    const float blackH = H * 0.62f;

    // black keys first (they sit on top)
    if (pos.y < blackH)
    {
        for (int n = kLoNote; n <= kHiNote; ++n)
        {
            if (! isBlackKey (n)) continue;
            const int wIdx = whiteKeyIndex (n); // white key to the left
            const float bx = (wIdx + 1) * whiteW;
            if (std::abs (pos.x - bx) < whiteW * 0.32f)
                return n;
        }
    }

    // white keys
    const int wIdx = juce::jlimit (0, numWhite - 1, (int) (pos.x / whiteW));
    int count = -1;
    for (int n = kLoNote; n <= kHiNote; ++n)
    {
        if (isBlackKey (n)) continue;
        if (++count == wIdx) return n;
    }
    return -1;
}

float NoctisKeyboard::velocityAt (int y) const
{
    // harder near the bottom of the key — piano feel
    const float t = juce::jlimit (0.0f, 1.0f, (float) y / juce::jmax (1, getHeight()));
    return 0.45f + 0.55f * t;
}

void NoctisKeyboard::noteOnMouse (int note, float velocity)
{
    if (note < 0 || mouseNotes[note]) return;
    mouseNotes[note] = true;
    heldMouseNote = note;
    processor.postMidiMessage (juce::MidiMessage::noteOn (1, note, velocity));
    repaint();
}

void NoctisKeyboard::noteOffMouse (int note)
{
    if (note < 0 || ! mouseNotes[note]) return;
    mouseNotes[note] = false;
    if (heldMouseNote == note) heldMouseNote = -1;
    processor.postMidiMessage (juce::MidiMessage::noteOff (1, note));
    repaint();
}

void NoctisKeyboard::mouseDown (const juce::MouseEvent& e)
{
    const int note = noteAtPosition (e.getPosition());
    if (note >= 0)
        noteOnMouse (note, velocityAt (e.y));
}

void NoctisKeyboard::mouseDrag (const juce::MouseEvent& e)
{
    const int note = noteAtPosition (e.getPosition());
    if (note != heldMouseNote)
    {
        if (heldMouseNote >= 0) noteOffMouse (heldMouseNote);
        if (note >= 0) noteOnMouse (note, velocityAt (e.y)); // glissando
    }
}

void NoctisKeyboard::mouseUp (const juce::MouseEvent&)
{
    if (heldMouseNote >= 0) noteOffMouse (heldMouseNote);
    // safety: release any stuck mouse notes
    for (int n = kLoNote; n <= kHiNote; ++n)
        if (mouseNotes[n]) noteOffMouse (n);
}

void NoctisKeyboard::timerCallback()
{
    bool notes[128] = {};
    processor.getActiveNotes (notes);
    bool changed = false;
    for (int i = 0; i < 128; ++i)
    {
        if (dawNotes[i] != notes[i]) { dawNotes[i] = notes[i]; changed = true; }
    }
    if (changed) repaint();
}

void NoctisKeyboard::paint (juce::Graphics& g)
{
    const int W = getWidth(), H = getHeight();
    if (W <= 0 || H <= 0) return;

    // backdrop: dark recessed well
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.fillRect (0, 0, W, H);

    int numWhite = 0;
    for (int n = kLoNote; n <= kHiNote; ++n)
        if (! isBlackKey (n)) ++numWhite;
    const float whiteW = (float) W / numWhite;
    const float blackH = H * 0.62f;
    const float blackW = whiteW * 0.64f;
    const auto glow = processor.getGlowColour();
    const auto glowDark = glow.darker (0.45f);

    // white keys: dark graphite, NOT white
    int wIdx = 0;
    for (int n = kLoNote; n <= kHiNote; ++n)
    {
        if (isBlackKey (n)) continue;
        const float x = wIdx * whiteW;
        const bool active = dawNotes[n] || mouseNotes[n];

        juce::ColourGradient keyGrad (NoctisColors::steelDark, 0, 0,
                                      juce::Colour (0xff14141b), 0, (float) H, false);
        if (active)
            keyGrad = juce::ColourGradient (glow, 0, 0, glowDark, 0, (float) H, false);
        g.setGradientFill (keyGrad);
        g.fillRect (x + 1.0f, 2.0f, whiteW - 2.0f, (float) H - 4.0f);

        // top bevel light
        g.setColour (juce::Colours::white.withAlpha (active ? 0.25f : 0.07f));
        g.fillRect (x + 1.0f, 2.0f, whiteW - 2.0f, 2.0f);

        if (active)
        {
            g.setColour (glow.brighter (0.3f).withAlpha (0.8f));
            g.drawRect (x + 1.0f, 2.0f, whiteW - 2.0f, (float) H - 4.0f, 1.5f);
        }
        else
        {
            g.setColour (juce::Colours::black.withAlpha (0.6f));
            g.drawRect (x + 1.0f, 2.0f, whiteW - 2.0f, (float) H - 4.0f, 1.0f);
        }
        ++wIdx;
    }

    // black keys: near-black slabs with reactive edge glow when active
    for (int n = kLoNote; n <= kHiNote; ++n)
    {
        if (! isBlackKey (n)) continue;
        const int leftWhite = whiteKeyIndex (n);
        const float x = (leftWhite + 1) * whiteW - blackW * 0.5f;
        const bool active = dawNotes[n] || mouseNotes[n];

        juce::ColourGradient blk (active ? glow : juce::Colour (0xff1c1c26),
                                  0, 0,
                                  active ? glowDark : juce::Colour (0xff060608),
                                  0, blackH, false);
        g.setGradientFill (blk);
        g.fillRoundedRectangle (x, 2, blackW, blackH - 2, 3.0f);

        g.setColour (juce::Colours::white.withAlpha (active ? 0.2f : 0.06f));
        g.fillRoundedRectangle (x + 2, 4, blackW - 4, 3, 1.5f);

        if (active)
        {
            g.setColour (glow.brighter (0.3f).withAlpha (0.9f));
            g.drawRoundedRectangle (x, 2, blackW, blackH - 2, 3.0f, 1.5f);
        }
    }

    // C labels on white keys
    g.setColour (NoctisColors::textDim.withAlpha (0.7f));
    g.setFont (juce::Font (9.0f, juce::Font::bold));
    wIdx = 0;
    for (int n = kLoNote; n <= kHiNote; ++n)
    {
        if (isBlackKey (n)) continue;
        if (n % 12 == 0) // C
        {
            const float x = wIdx * whiteW;
            g.drawText ("C" + juce::String (n / 12 - 1),
                        (int) x, H - 20, (int) whiteW, 14, juce::Justification::centred);
        }
        ++wIdx;
    }
}

//==============================================================================
// NoctisEditor
//==============================================================================
NoctisEditor::NoctisEditor (NoctisProcessor& p)
    : AudioProcessorEditor (p), processor (p), lnf (p), xyPad (p), vuMeter (p), keyboard (p)
{
    setLookAndFeel (&lnf);
    setSize (1060, 740);

    factoryPresets = getNoctisFactoryPresets();
    scanUserPresets();

    // logo (small brand mark in header)
    logoImage = juce::ImageCache::getFromMemory (BinaryData::headroomlogo_png,
                                                 BinaryData::headroomlogo_pngSize);

    // preset browser (factory + user)
    presetBox.setName ("presets");
    refreshPresetBox();
    presetBox.onChange = [this]
    {
        const int idx = presetBox.getSelectedId() - 1;
        if (presetAtCombinedIndex (idx) != nullptr)
            loadPresetByIndex (idx);
    };
    addAndMakeVisible (presetBox);

    heartButton.setName ("heart");
    heartButton.onClick = [this]
    {
        if (const auto* pr = presetAtCombinedIndex (currentPresetIndex))
        {
            processor.toggleFavorite (pr->name);
            updateHeart();
            refreshPresetBox(); // refresh ♥ markers
        }
    };
    addAndMakeVisible (heartButton);

    saveButton.onClick = [this]
    {
        auto* alert = new juce::AlertWindow ("Save Preset", "Name your preset:",
                                            juce::AlertWindow::NoIcon);
        alert->addTextEditor ("name", "My Preset", "Preset name:");
        alert->addButton ("Save", 1, juce::KeyPress (juce::KeyPress::returnKey));
        alert->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));
        // capture the editor safely; the alert deletes itself on exit
        alert->enterModalState (true, juce::ModalCallbackFunction::create (
            [safeThis = juce::Component::SafePointer<NoctisEditor> (this), alert] (int result)
            {
                if (result == 1 && safeThis != nullptr)
                {
                    const juce::String name = alert->getTextEditorContents ("name").trim();
                    if (name.isNotEmpty())
                        safeThis->saveUserPreset (name);
                }
                delete alert;
            }), false);
    };
    addAndMakeVisible (saveButton);

    diceButton.setName ("dice");
    diceButton.onClick = [this] { onDicePressed(); };
    addAndMakeVisible (diceButton);

    auto numPresets = [this]() -> int
    {
        return (int) (factoryPresets.size() + userPresets.size() + dicePresets.size());
    };
    prevButton.onClick = [this, numPresets]
    {
        const int n = numPresets();
        if (n == 0) return;
        int idx = currentPresetIndex - 1;
        if (idx < 0) idx = n - 1;
        presetBox.setSelectedId (idx + 1, juce::sendNotification);
    };
    nextButton.onClick = [this, numPresets]
    {
        const int n = numPresets();
        if (n == 0) return;
        int idx = (currentPresetIndex + 1) % n;
        presetBox.setSelectedId (idx + 1, juce::sendNotification);
    };
    addAndMakeVisible (prevButton);
    addAndMakeVisible (nextButton);

    // playable keyboard strip
    addAndMakeVisible (keyboard);

    // hero XY pad + VU
    addAndMakeVisible (xyPad);
    addAndMakeVisible (vuMeter);

    // wave selectors
    juce::StringArray waveNames;
    for (int i = 0; i < NoctisSynthEngine::kNumWaveforms; ++i)
        waveNames.add (NoctisSynthEngine::waveformNames[i]);
    osc1WaveBox.addItemList (waveNames, 1);
    osc2WaveBox.addItemList (waveNames, 1);
    osc1WaveBox.setSelectedId (2, juce::dontSendNotification); // Saw
    osc2WaveBox.setSelectedId (2, juce::dontSendNotification);
    auto waveChanged = [this] (juce::ComboBox* box, const char* paramID)
    {
        if (auto* param = dynamic_cast<juce::AudioParameterChoice*> (
                processor.apvts.getParameter (paramID)))
            param->setValueNotifyingHost (param->convertTo0to1 ((float) (box->getSelectedId() - 1)));
    };
    osc1WaveBox.onChange = [&, waveChanged] { waveChanged (&osc1WaveBox, NoctisProcessor::kOsc1Wave); };
    osc2WaveBox.onChange = [&, waveChanged] { waveChanged (&osc2WaveBox, NoctisProcessor::kOsc2Wave); };
    addAndMakeVisible (osc1WaveBox);
    addAndMakeVisible (osc2WaveBox);
    osc1Label.setText ("OSC 1", juce::dontSendNotification);
    osc2Label.setText ("OSC 2", juce::dontSendNotification);
    for (auto* l : { &osc1Label, &osc2Label })
    {
        l->setColour (juce::Label::textColourId, NoctisColors::textDim);
        l->setJustificationType (juce::Justification::centred);
        addAndMakeVisible (l);
    }

    // knobs — labels go UNDERNEATH, never on the knob
    addKnob (NoctisProcessor::kCutoff,     "CUTOFF");
    addKnob (NoctisProcessor::kResonance,  "RESONANCE");
    addKnob (NoctisProcessor::kOscMix,     "OSC MIX");
    addKnob (NoctisProcessor::kSubLevel,   "SUB LEVEL");
    addKnob (NoctisProcessor::kDrive,      "DRIVE");
    addKnob (NoctisProcessor::kChorus,     "CHORUS");
    addKnob (NoctisProcessor::kReverb,     "REVERB");
    addKnob (NoctisProcessor::kDelayMix,   "DELAY");
    addKnob (NoctisProcessor::kBitcrush,   "CRUSH");
    addKnob (NoctisProcessor::kAmpA,       "ATTACK");
    addKnob (NoctisProcessor::kAmpD,       "DECAY");
    addKnob (NoctisProcessor::kAmpS,       "SUSTAIN");
    addKnob (NoctisProcessor::kAmpR,       "RELEASE");
    addKnob (NoctisProcessor::kGlide,      "GLIDE");

    // load the first preset by default (Deep Sub 808 — shows off the weight)
    if (! factoryPresets.empty())
    {
        presetBox.setSelectedId (1, juce::sendNotification);
    }

    // watch for tweaks -> mark preset modified
    processor.apvts.addParameterListener (NoctisProcessor::kCutoff, this);
    processor.apvts.addParameterListener (NoctisProcessor::kResonance, this);

    startTimerHz (10);
}

NoctisEditor::~NoctisEditor()
{
    processor.apvts.removeParameterListener (NoctisProcessor::kCutoff, this);
    processor.apvts.removeParameterListener (NoctisProcessor::kResonance, this);
    setLookAndFeel (nullptr);
}

void NoctisEditor::addKnob (const juce::String& paramID, const juce::String& labelText)
{
    auto ku = std::make_unique<KnobUnit>();
    ku->slider.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    ku->slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    ku->slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f,
                                    juce::MathConstants<float>::pi * 2.75f, true);
    ku->attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processor.apvts, paramID, ku->slider);
    ku->label.setText (labelText, juce::dontSendNotification);
    ku->label.setColour (juce::Label::textColourId, NoctisColors::textDim);
    ku->label.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (ku->slider);
    addAndMakeVisible (ku->label);
    knobs.push_back (std::move (ku));
}

const NoctisPreset* NoctisEditor::presetAtCombinedIndex (int idx) const
{
    if (idx < 0) return nullptr;
    if (idx < (int) factoryPresets.size())
        return &factoryPresets[(size_t) idx];
    const int uIdx = idx - (int) factoryPresets.size();
    if (uIdx < (int) userPresets.size())
        return &userPresets[(size_t) uIdx];
    const int dIdx = uIdx - (int) userPresets.size();
    if (dIdx < (int) dicePresets.size())
        return &dicePresets[(size_t) dIdx];
    return nullptr;
}

void NoctisEditor::refreshPresetBox()
{
    presetBox.clear (juce::dontSendNotification);
    for (size_t i = 0; i < factoryPresets.size(); ++i)
    {
        juce::String item = factoryPresets[i].name;
        if (processor.isFavorite (factoryPresets[i].name))
            item = "♥ " + item;
        presetBox.addItem (item, (int) i + 1);
    }
    if (! userPresets.empty())
    {
        presetBox.addSeparator();
        for (size_t i = 0; i < userPresets.size(); ++i)
        {
            juce::String item = userPresets[i].name;
            if (processor.isFavorite (userPresets[i].name))
                item = "♥ " + item;
            presetBox.addItem (item, (int) (factoryPresets.size() + i) + 1);
        }
    }
    if (! dicePresets.empty())
    {
        presetBox.addSeparator();
        for (size_t i = 0; i < dicePresets.size(); ++i)
        {
            juce::String item = juce::String ("🎲 ") + dicePresets[i].name;
            if (processor.isFavorite (dicePresets[i].name))
                item = "♥ " + item;
            presetBox.addItem (item, (int) (factoryPresets.size() + userPresets.size() + i) + 1);
        }
    }
    // explicit IDs: factory 1..F, user F+1.., dice F+U+1.., so selection by ID is stable
    if (currentPresetIndex >= 0)
        presetBox.setSelectedId (currentPresetIndex + 1, juce::dontSendNotification);
}

void NoctisEditor::loadPresetByIndex (int index)
{
    const auto* pr = presetAtCombinedIndex (index);
    if (pr == nullptr) return;
    currentPresetIndex = index;
    processor.applyPreset (*pr);
    osc1WaveBox.setSelectedId (pr->osc1Wave + 1, juce::dontSendNotification);
    osc2WaveBox.setSelectedId (pr->osc2Wave + 1, juce::dontSendNotification);
    updateHeart();
}

void NoctisEditor::onDicePressed()
{
    // Current UI state feeds MUTATE mode; factory list feeds CROSSBREED.
    const auto current = currentStateAsPreset ("Current");
    auto dice = randomizer.generate (current, factoryPresets);

    dicePresets.push_back (dice);
    if (dicePresets.size() > 20)
        dicePresets.erase (dicePresets.begin()); // cap history

    currentPresetIndex = (int) (factoryPresets.size() + userPresets.size() + dicePresets.size()) - 1;
    refreshPresetBox(); // rebuilds list (with dice section) + selects new index
    loadPresetByIndex (currentPresetIndex); // applies preset, updates wave boxes + heart
}

void NoctisEditor::updateHeart()
{
    if (const auto* pr = presetAtCombinedIndex (currentPresetIndex))
    {
        const bool fav = processor.isFavorite (pr->name);
        heartButton.setToggleState (fav, juce::dontSendNotification);
        heartButton.setColour (juce::TextButton::textColourOnId, NoctisColors::heartRed);
        heartButton.setColour (juce::TextButton::textColourOffId, NoctisColors::textDim);
    }
}

//==============================================================================
// User presets: ~/Documents/Headroom Audio/Noctis/Presets/*.noctis
//==============================================================================
juce::File NoctisEditor::getPresetDir()
{
    return juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
               .getChildFile ("Headroom Audio/Noctis/Presets");
}

void NoctisEditor::scanUserPresets()
{
    userPresets.clear();
    const juce::File dir = getPresetDir();
    if (dir.isDirectory())
        for (auto& f : dir.findChildFiles (juce::File::findFiles, false, "*.noctis"))
            userPresets.push_back (presetFromFile (f));
}

NoctisPreset NoctisEditor::currentStateAsPreset (const juce::String& name)
{
    NoctisPreset p;
    p.name = name;
    p.factory = false;
    auto getF = [&] (const char* id)
    { return processor.apvts.getRawParameterValue (id)->load(); };

    p.osc1Wave = (int) getF (NoctisProcessor::kOsc1Wave);
    p.osc2Wave = (int) getF (NoctisProcessor::kOsc2Wave);
    p.oscMix = getF (NoctisProcessor::kOscMix);
    p.subLevel = getF (NoctisProcessor::kSubLevel);
    p.subWave = (int) getF (NoctisProcessor::kSubWave);
    p.cutoff = getF (NoctisProcessor::kCutoff);
    p.resonance = getF (NoctisProcessor::kResonance);
    p.filterEnvAmt = getF (NoctisProcessor::kFilterEnv);
    p.ampA = getF (NoctisProcessor::kAmpA);
    p.ampD = getF (NoctisProcessor::kAmpD);
    p.ampS = getF (NoctisProcessor::kAmpS);
    p.ampR = getF (NoctisProcessor::kAmpR);
    p.filtA = getF (NoctisProcessor::kFiltA);
    p.filtD = getF (NoctisProcessor::kFiltD);
    p.filtS = getF (NoctisProcessor::kFiltS);
    p.filtR = getF (NoctisProcessor::kFiltR);
    p.lfoTarget = (int) getF (NoctisProcessor::kLfoTarget);
    p.lfoRate = getF (NoctisProcessor::kLfoRate);
    p.lfoDepth = getF (NoctisProcessor::kLfoDepth);
    p.glide = getF (NoctisProcessor::kGlide);
    p.drive = getF (NoctisProcessor::kDrive);
    p.chorus = getF (NoctisProcessor::kChorus);
    p.masterGain = getF (NoctisProcessor::kMasterGain);
    return p;
}

void NoctisEditor::saveUserPreset (const juce::String& name)
{
    const juce::File dir = getPresetDir();
    dir.createDirectory();
    juce::String safe = name.replaceCharacter ('/', '_').replaceCharacter ('\\', '_');

    const auto p = currentStateAsPreset (safe);
    juce::ValueTree tree ("NoctisPreset");
    tree.setProperty ("name", p.name, nullptr);
    tree.setProperty ("osc1Wave", p.osc1Wave, nullptr);
    tree.setProperty ("osc2Wave", p.osc2Wave, nullptr);
    tree.setProperty ("oscMix", p.oscMix, nullptr);
    tree.setProperty ("subLevel", p.subLevel, nullptr);
    tree.setProperty ("subWave", p.subWave, nullptr);
    tree.setProperty ("cutoff", p.cutoff, nullptr);
    tree.setProperty ("resonance", p.resonance, nullptr);
    tree.setProperty ("filterEnvAmt", p.filterEnvAmt, nullptr);
    tree.setProperty ("ampA", p.ampA, nullptr);
    tree.setProperty ("ampD", p.ampD, nullptr);
    tree.setProperty ("ampS", p.ampS, nullptr);
    tree.setProperty ("ampR", p.ampR, nullptr);
    tree.setProperty ("filtA", p.filtA, nullptr);
    tree.setProperty ("filtD", p.filtD, nullptr);
    tree.setProperty ("filtS", p.filtS, nullptr);
    tree.setProperty ("filtR", p.filtR, nullptr);
    tree.setProperty ("lfoTarget", p.lfoTarget, nullptr);
    tree.setProperty ("lfoRate", p.lfoRate, nullptr);
    tree.setProperty ("lfoDepth", p.lfoDepth, nullptr);
    tree.setProperty ("glide", p.glide, nullptr);
    tree.setProperty ("drive", p.drive, nullptr);
    tree.setProperty ("chorus", p.chorus, nullptr);
    tree.setProperty ("masterGain", p.masterGain, nullptr);

    if (auto xml = tree.createXml())
        xml->writeTo (dir.getChildFile (safe + ".noctis"));

    scanUserPresets();
    refreshPresetBox();
    // select the newly saved preset
    currentPresetIndex = (int) (factoryPresets.size() + userPresets.size()) - 1;
    refreshPresetBox();
    processor.setCurrentPresetName (safe);
    updateHeart();
}

NoctisPreset NoctisEditor::presetFromFile (const juce::File& f)
{
    NoctisPreset p;
    p.name = f.getFileNameWithoutExtension();
    p.factory = false;
    if (auto xml = juce::XmlDocument::parse (f))
    {
        juce::ValueTree tree = juce::ValueTree::fromXml (*xml);
        if (tree.isValid() && tree.hasType ("NoctisPreset"))
        {
            p.osc1Wave = (int) tree.getProperty ("osc1Wave", 1);
            p.osc2Wave = (int) tree.getProperty ("osc2Wave", 1);
            p.oscMix = (float) tree.getProperty ("oscMix", 50.0);
            p.subLevel = (float) tree.getProperty ("subLevel", 0.0);
            p.subWave = (int) tree.getProperty ("subWave", 0);
            p.cutoff = (float) tree.getProperty ("cutoff", 8000.0);
            p.resonance = (float) tree.getProperty ("resonance", 10.0);
            p.filterEnvAmt = (float) tree.getProperty ("filterEnvAmt", 0.0);
            p.ampA = (float) tree.getProperty ("ampA", 5.0);
            p.ampD = (float) tree.getProperty ("ampD", 100.0);
            p.ampS = (float) tree.getProperty ("ampS", 80.0);
            p.ampR = (float) tree.getProperty ("ampR", 150.0);
            p.filtA = (float) tree.getProperty ("filtA", 5.0);
            p.filtD = (float) tree.getProperty ("filtD", 100.0);
            p.filtS = (float) tree.getProperty ("filtS", 50.0);
            p.filtR = (float) tree.getProperty ("filtR", 150.0);
            p.lfoTarget = (int) tree.getProperty ("lfoTarget", 0);
            p.lfoRate = (float) tree.getProperty ("lfoRate", 5.0);
            p.lfoDepth = (float) tree.getProperty ("lfoDepth", 0.0);
            p.glide = (float) tree.getProperty ("glide", 0.0);
            p.drive = (float) tree.getProperty ("drive", 0.0);
            p.chorus = (float) tree.getProperty ("chorus", 0.0);
            p.masterGain = (float) tree.getProperty ("masterGain", 80.0);
        }
    }
    return p;
}

void NoctisEditor::markModified()
{
    // append "*" to preset name display when tweaked — light touch, no box rebuild
}

void NoctisEditor::parameterChanged (const juce::String&, float) {}

void NoctisEditor::timerCallback() {}

//==============================================================================
void NoctisEditor::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();

    // --- brushed black-metal chassis ---
    juce::ColourGradient chassis (NoctisColors::chassisLight, 0, 0,
                                  NoctisColors::chassisDark, 0, bounds.getHeight(), false);
    g.setGradientFill (chassis);
    g.fillRect (bounds);

    // brushed texture: faint horizontal lines
    g.setColour (juce::Colours::white.withAlpha (0.016f));
    for (int y = 0; y < getHeight(); y += 3)
        g.drawHorizontalLine (y, 0, (float) getWidth());

    // AUDIO-REACTIVE ambient glow (top) + cool fill (bottom) — the plugin breathes
    const auto glow = processor.getGlowColour();
    juce::ColourGradient glowTop (glow.withAlpha (0.10f), 0, 0,
                                  juce::Colours::transparentBlack, 0, 220.0f, false);
    g.setGradientFill (glowTop);
    g.fillRect (0, 0, getWidth(), 220);
    juce::ColourGradient glowBottom (juce::Colours::transparentBlack, 0, getHeight() - 200.0f,
                                     glow.withAlpha (0.07f), 0, (float) getHeight(), false);
    g.setGradientFill (glowBottom);
    g.fillRect (0, getHeight() - 200, getWidth(), 200);

    // --- header strip ---
    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.fillRect (0, 0, getWidth(), 64);

    // NOCTIS wordmark: bold condensed, letterspaced, with reactive underglow
    g.setColour (glow.withAlpha (0.45f));
    juce::Font __kf (40.0f, juce::Font::bold);
    __kf.setExtraKerningFactor (0.22f);
    g.setFont (__kf);
    g.drawText ("NOCTIS", 24, 16, 220, 40, juce::Justification::left);
    g.setColour (NoctisColors::textMain);
    g.setFont (__kf);
    g.drawText ("NOCTIS", 24, 14, 220, 40, juce::Justification::left);

    // logo (small brand mark)
    if (logoImage.isValid())
        g.drawImage (logoImage, 250, 12, 40, 40,
                     0, 0, logoImage.getWidth(), logoImage.getHeight());

    // tagline
    g.setColour (NoctisColors::textDim);
    juce::Font __kf2 (10.0f);
    __kf2.setExtraKerningFactor (0.18f);
    g.setFont (__kf2);
    g.drawText ("DARK  WAVETABLE  SYNTH", 300, 24, 260, 16, juce::Justification::left);

    // --- corner screws ---
    auto drawScrew = [&] (float sx, float sy)
    {
        g.setColour (NoctisColors::steelDark);
        g.fillEllipse (sx - 7, sy - 7, 14, 14);
        g.setColour (NoctisColors::steelLight.withAlpha (0.6f));
        g.drawEllipse (sx - 7, sy - 7, 14, 14, 1.2f);
        g.setColour (juce::Colours::black.withAlpha (0.7f));
        g.drawLine (sx - 4, sy - 4, sx + 4, sy + 4, 1.6f);
    };
    drawScrew (18, getHeight() - 18);
    drawScrew (getWidth() - 18, getHeight() - 18);
    drawScrew (18, 82);
    drawScrew (getWidth() - 18, 82);

    // --- section dividers ---
    g.setColour (glow.withAlpha (0.25f));
    g.drawHorizontalLine (64, 0, (float) getWidth());
    // divider above the keyboard strip
    g.setColour (NoctisColors::purple.withAlpha (0.18f));
    g.drawHorizontalLine (getHeight() - 92, 12, (float) getWidth() - 12);
}

//==============================================================================
void NoctisEditor::resized()
{
    const int W = getWidth();
    const int keyboardH = 92;
    const int mainBottom = getHeight() - keyboardH;

    // header: preset browser cluster (right side)
    presetBox.setBounds (W - 532, 16, 200, 32);
    diceButton.setBounds (W - 324, 16, 84, 32);
    prevButton.setBounds (W - 232, 16, 32, 32);
    nextButton.setBounds (W - 196, 16, 32, 32);
    heartButton.setBounds (W - 156, 16, 44, 32);
    saveButton.setBounds (W - 104, 16, 76, 32);

    // hero XY pad (left)
    const int padX = 24, padY = 84, padSize = 380;
    xyPad.setBounds (padX, padY, padSize, padSize);

    // wave selectors under the pad
    osc1Label.setBounds (padX, padY + padSize + 8, 180, 16);
    osc1WaveBox.setBounds (padX, padY + padSize + 26, 180, 30);
    osc2Label.setBounds (padX + 200, padY + padSize + 8, 180, 16);
    osc2WaveBox.setBounds (padX + 200, padY + padSize + 26, 180, 30);

    // VU meters (right of pad, top)
    const int rightX = padX + padSize + 24;
    vuMeter.setBounds (rightX, padY, W - rightX - 24, 104);

    // knob grid: 6 per row, 2 rows — labels underneath each knob
    const int knobAreaX = rightX;
    const int knobAreaW = W - rightX - 24;
    const int cols = 6;
    const int knobCellW = knobAreaW / cols;
    const int knobSize = juce::jmin (68, knobCellW - 12);
    const int rowY1 = padY + 120;
    const int rowY2 = rowY1 + 104;

    for (size_t i = 0; i < knobs.size(); ++i)
    {
        const int row = (int) i / cols;
        const int col = (int) i % cols;
        const int kx = knobAreaX + col * knobCellW + (knobCellW - knobSize) / 2;
        const int ky = (row == 0 ? rowY1 : rowY2);
        knobs[i]->slider.setBounds (kx, ky, knobSize, knobSize);
        knobs[i]->label.setBounds (kx - 10, ky + knobSize + 2, knobSize + 20, 18);
    }

    // piano keyboard strip along the bottom
    keyboard.setBounds (12, mainBottom + 8, W - 24, keyboardH - 16);
}
