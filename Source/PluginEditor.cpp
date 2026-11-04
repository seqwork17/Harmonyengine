#include "PluginEditor.h"

using namespace hf;
using namespace hf::ui;

static juce::Font fnt (float h, bool bold = false)
{
    return juce::Font (juce::FontOptions (h, bold ? juce::Font::bold : juce::Font::plain));
}

static juce::String noteName (int midi)
{
    return juce::String (kNoteNames[((midi % 12) + 12) % 12]) + juce::String (midi / 12 - 1);
}

// ============================================================ LookAndFeel
LookAndFeel::LookAndFeel()
{
    setColour (juce::Slider::textBoxTextColourId, col::text);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::Label::textColourId, col::dim);
    setColour (juce::ComboBox::backgroundColourId, col::panel2);
    setColour (juce::ComboBox::textColourId, col::text);
    setColour (juce::ComboBox::outlineColourId, col::edge);
    setColour (juce::ComboBox::arrowColourId, col::accent);
    setColour (juce::PopupMenu::backgroundColourId, col::panel);
    setColour (juce::PopupMenu::textColourId, col::text);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, col::accent.withAlpha (0.25f));
    setColour (juce::PopupMenu::highlightedTextColourId, col::text);
}

void LookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float start, float end, juce::Slider& s)
{
    const auto b = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (3.f);
    const float r = std::min (b.getWidth(), b.getHeight()) * 0.5f;
    const auto c = b.getCentre();
    const auto colour = s.findColour (juce::Slider::rotarySliderFillColourId);

    juce::ColourGradient body (juce::Colour (0xff2a3254), c.x, c.y - r, juce::Colour (0xff0c0f1c), c.x, c.y + r, false);
    g.setGradientFill (body);
    g.fillEllipse (c.x - r * 0.7f, c.y - r * 0.7f, r * 1.4f, r * 1.4f);
    g.setColour (col::edge);
    g.drawEllipse (c.x - r * 0.7f, c.y - r * 0.7f, r * 1.4f, r * 1.4f, 1.f);

    juce::Path track;
    track.addCentredArc (c.x, c.y, r * 0.9f, r * 0.9f, 0.f, start, end, true);
    g.setColour (juce::Colour (0xff232a47));
    g.strokePath (track, juce::PathStrokeType (3.f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    const float ang = start + pos * (end - start);
    juce::Path val;
    val.addCentredArc (c.x, c.y, r * 0.9f, r * 0.9f, 0.f, start, ang, true);
    g.setColour (colour.withAlpha (0.22f));
    g.strokePath (val, juce::PathStrokeType (8.f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour (colour);
    g.strokePath (val, juce::PathStrokeType (3.f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    juce::Path p;
    p.addRoundedRectangle (-1.5f, -r * 0.66f, 3.f, r * 0.28f, 1.5f);
    g.setColour (juce::Colours::white);
    g.fillPath (p, juce::AffineTransform::rotation (ang).translated (c.x, c.y));
}

void LookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float, float,
                                    juce::Slider::SliderStyle style, juce::Slider& s)
{
    if (style != juce::Slider::LinearVertical) return;
    const auto colour = s.findColour (juce::Slider::trackColourId);
    const float cx = (float) x + (float) w * 0.5f;
    const float top = (float) y + 6.f, bottom = (float) (y + h) - 6.f;

    g.setColour (juce::Colour (0xff0a0d18));
    g.fillRoundedRectangle (cx - 4.f, top, 8.f, bottom - top, 4.f);
    g.setColour (colour.withAlpha (0.25f));
    g.fillRoundedRectangle (cx - 6.f, pos, 12.f, bottom - pos, 6.f);
    juce::ColourGradient fill (colour, cx, pos, colour.darker (0.8f), cx, bottom, false);
    g.setGradientFill (fill);
    g.fillRoundedRectangle (cx - 3.f, pos, 6.f, bottom - pos, 3.f);

    const juce::Rectangle<float> thumb (cx - 13.f, pos - 6.f, 26.f, 12.f);
    g.setColour (juce::Colour (0xff2b3558));
    g.fillRoundedRectangle (thumb, 4.f);
    g.setColour (colour);
    g.drawRoundedRectangle (thumb, 4.f, 1.2f);
    g.fillRect (thumb.getCentreX() - 8.f, thumb.getCentreY() - 0.5f, 16.f, 1.f);
}

void LookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool highlighted, bool)
{
    const auto r = b.getLocalBounds().toFloat().reduced (1.5f);
    const auto c = b.findColour (juce::ToggleButton::tickColourId);
    const bool on = b.getToggleState();
    if (on)
    {
        g.setColour (c.withAlpha (0.25f));
        g.fillRoundedRectangle (r.expanded (1.5f), 7.f);
    }
    g.setColour (on ? c.withAlpha (0.85f) : (highlighted ? col::panel2.brighter (0.2f) : col::panel2));
    g.fillRoundedRectangle (r, 6.f);
    g.setColour (on ? c : col::edge);
    g.drawRoundedRectangle (r, 6.f, 1.2f);
    g.setColour (on ? juce::Colour (0xff06080f) : col::dim);
    g.setFont (fnt (juce::jmin (12.f, r.getHeight() * 0.62f), true));
    g.drawText (b.getButtonText(), r, juce::Justification::centred);
}

void LookAndFeel::drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox& box)
{
    const auto r = juce::Rectangle<float> (0.f, 0.f, (float) w, (float) h).reduced (0.5f);
    g.setColour (col::panel2);
    g.fillRoundedRectangle (r, 6.f);
    g.setColour (box.hasKeyboardFocus (true) ? col::accent : col::edge);
    g.drawRoundedRectangle (r, 6.f, 1.2f);
    juce::Path arrow;
    const float ax = (float) w - 14.f, ay = (float) h * 0.5f;
    arrow.addTriangle (ax - 4.f, ay - 2.f, ax + 4.f, ay - 2.f, ax, ay + 3.f);
    g.setColour (col::accent);
    g.fillPath (arrow);
}

// ============================================================ Componentes simples
Knob::Knob (juce::AudioProcessorValueTreeState& s, const juce::String& id, const juce::String& name, juce::Colour c, int textW)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, textW, 14);
    slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.2f, juce::MathConstants<float>::pi * 2.8f, true);
    slider.setColour (juce::Slider::rotarySliderFillColourId, c);
    slider.setDoubleClickReturnValue (true, s.getParameter (id)->convertFrom0to1 (s.getParameter (id)->getDefaultValue()));
    addAndMakeVisible (slider);
    label.setText (name, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setFont (fnt (11.f, true));
    addAndMakeVisible (label);
    att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (s, id, slider);
}

void Knob::resized()
{
    auto r = getLocalBounds();
    label.setBounds (r.removeFromTop (14));
    slider.setBounds (r);
}

Selector::Selector (juce::AudioProcessorValueTreeState& s, const juce::String& id)
{
    if (auto* p = dynamic_cast<juce::AudioParameterChoice*> (s.getParameter (id)))
        box.addItemList (p->choices, 1);
    addAndMakeVisible (box);
    att = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (s, id, box);
}

Toggle::Toggle (juce::AudioProcessorValueTreeState& s, const juce::String& id, const juce::String& text, juce::Colour c)
{
    btn.setButtonText (text);
    btn.setColour (juce::ToggleButton::tickColourId, c);
    addAndMakeVisible (btn);
    att = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (s, id, btn);
}

void MeterBar::tick()
{
    const float v = source.load();
    const float db = juce::Decibels::gainToDecibels (v, -60.f);
    const float target = juce::jlimit (0.f, 1.f, (db + 60.f) / 60.f);
    const float next = target > level ? target : level + (target - level) * 0.25f;
    if (std::abs (next - level) > 0.003f) { level = next; repaint(); }
}

void MeterBar::paint (juce::Graphics& g)
{
    const auto r = getLocalBounds().toFloat();
    g.setColour (juce::Colour (0xff0a0d18));
    g.fillRoundedRectangle (r, 3.f);
    const float h = r.getHeight() * level;
    juce::ColourGradient grad (col::accent2, 0.f, r.getY(), colour, 0.f, r.getBottom(), false);
    grad.addColour (0.25, col::gold);
    g.setGradientFill (grad);
    g.fillRoundedRectangle (r.getX() + 1.f, r.getBottom() - h, r.getWidth() - 2.f, h, 2.f);
}

// ============================================================ PitchView
PitchView::PitchView (HarmonyForgeProcessor& p) : proc (p)
{
    for (int i = 0; i < N; ++i)
    {
        detected[i] = target[i] = -1.f;
        for (int v = 0; v < hf::kNumVoices; ++v) { voices[v][i] = -1.f; voiceOn[v][i] = false; }
    }
}

void PitchView::tick()
{
    auto& e = proc.engine;
    head = (head + 1) % N;
    detected[head] = e.telDetected.load();
    target[head] = e.telTarget.load();
    for (int v = 0; v < hf::kNumVoices; ++v)
    {
        voices[v][head] = e.telVoice[v].load();
        voiceOn[v][head] = e.telVoiceOn[v].load() > 0.5f;
    }
    if (detected[head] > 0.f)
        viewCentre += (detected[head] - viewCentre) * 0.04f;
    repaint();
}

void PitchView::paint (juce::Graphics& g)
{
    const auto area = getLocalBounds().toFloat();
    g.setColour (col::panel);
    g.fillRoundedRectangle (area, 10.f);
    g.setColour (col::edge);
    g.drawRoundedRectangle (area.reduced (0.5f), 10.f, 1.f);

    const float gutter = 38.f;
    const auto plot = area.reduced (6.f).withTrimmedLeft (gutter).withTrimmedTop (22.f);
    const float range = 30.f;
    const float lo = viewCentre - range * 0.5f;
    auto yOf = [&] (float midi) { return plot.getBottom() - (midi - lo) / range * plot.getHeight(); };

    const uint16_t mask = proc.currentScaleMask();
    g.saveState();
    g.reduceClipRegion (area.reduced (2.f).toNearestInt());
    for (int n = (int) std::floor (lo) - 1; n <= (int) std::ceil (lo + range) + 1; ++n)
    {
        const float yc = yOf ((float) n);
        const float rowH = plot.getHeight() / range;
        const juce::Rectangle<float> row (plot.getX(), yc - rowH * 0.5f, plot.getWidth(), rowH);
        const bool allowed = mask == 0 || isAllowed (n, mask);
        const bool black = isAllowed (n, hf::maskFrom ({ 1, 3, 6, 8, 10 }));
        if (allowed) { g.setColour (col::accent.withAlpha (0.07f)); g.fillRect (row); }
        g.setColour (col::edge.withAlpha (allowed ? 0.7f : 0.25f));
        g.drawHorizontalLine ((int) yc, plot.getX(), plot.getRight());
        g.setColour (black ? col::dim.withAlpha (0.5f) : col::dim);
        g.setFont (fnt (10.f));
        if (allowed || n % 12 == 0)
            g.drawText (noteName (n), (int) area.getX() + 6, (int) (yc - 6.f), (int) gutter - 4, 12, juce::Justification::centredLeft);
    }

    auto trace = [&] (const float* data, const bool* on, juce::Colour c, float thick, float alpha)
    {
        juce::Path path;
        bool pen = false;
        for (int i = 0; i < N; ++i)
        {
            const int idx = (head + 1 + i) % N;
            const float m = data[idx];
            const bool valid = m > 0.f && (on == nullptr || on[idx]);
            const float x = plot.getX() + plot.getWidth() * (float) i / (float) (N - 1);
            if (! valid) { pen = false; continue; }
            if (! pen) { path.startNewSubPath (x, yOf (m)); pen = true; }
            else path.lineTo (x, yOf (m));
        }
        g.setColour (c.withAlpha (alpha * 0.3f));
        g.strokePath (path, juce::PathStrokeType (thick + 3.f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (c.withAlpha (alpha));
        g.strokePath (path, juce::PathStrokeType (thick, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    };

    for (int v = 0; v < hf::kNumVoices; ++v)
        trace (voices[v], voiceOn[v], col::voice (v), 1.4f, 0.85f);
    trace (target, nullptr, col::accent, 2.2f, 1.f);
    trace (detected, nullptr, juce::Colours::white, 1.2f, 0.55f);
    g.restoreState();

    // Lectura numérica
    const float m = detected[head];
    g.setFont (fnt (14.f, true));
    if (m > 0.f)
    {
        const int nn = (int) std::lround (m);
        const int cents = (int) std::lround ((m - (float) nn) * 100.f);
        g.setColour (col::text);
        g.drawText (noteName (nn) + "  " + (cents >= 0 ? "+" : "") + juce::String (cents) + " ct",
                    (int) area.getX() + 12, (int) area.getY() + 4, 160, 18, juce::Justification::centredLeft);
        const float tg = target[head];
        g.setColour (col::accent);
        g.drawText (juce::String::fromUTF8 ("\xe2\x86\x92 ") + noteName ((int) std::lround (tg)),
                    (int) area.getX() + 150, (int) area.getY() + 4, 100, 18, juce::Justification::centredLeft);
    }
    else
    {
        g.setColour (col::dim);
        g.drawText ("Sing something...", (int) area.getX() + 12, (int) area.getY() + 4, 200, 18, juce::Justification::centredLeft);
    }
}

// ============================================================ VoiceStrip
VoiceStrip::VoiceStrip (HarmonyForgeProcessor& p, int index)
    : proc (p), idx (index), colour (col::voice (index)),
      enable (p.apvts, pid::voice (index, pid::vEnable), "VOICE " + juce::String (index + 1), col::voice (index)),
      pan (p.apvts, pid::voice (index, pid::vPan), "PAN", col::voice (index), 44),
      interval (p.apvts, pid::voice (index, pid::vDeg), "INTERVAL", col::voice (index), 44),
      octave (p.apvts, pid::voice (index, pid::vOct), "OCTAVE", col::voice (index), 44),
      semis (p.apvts, pid::voice (index, pid::vSemi), "SEMI", col::voice (index), 44),
      fine (p.apvts, pid::voice (index, pid::vCents), "FINE", col::voice (index), 44)
{
    addAndMakeVisible (enable);
    fader.setSliderStyle (juce::Slider::LinearVertical);
    fader.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 46, 14);
    fader.setColour (juce::Slider::trackColourId, colour);
    fader.setDoubleClickReturnValue (true, -4.0);
    addAndMakeVisible (fader);
    faderAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (p.apvts, pid::voice (index, pid::vVol), fader);
    for (auto* k : { &pan, &interval, &octave, &semis, &fine }) addAndMakeVisible (k);
}

void VoiceStrip::tick()
{
    const float target = proc.engine.telVoiceOn[idx].load();
    const float next = activity + (target - activity) * 0.2f;
    if (std::abs (next - activity) > 0.01f) { activity = next; repaint(); }
}

void VoiceStrip::paint (juce::Graphics& g)
{
    const auto r = getLocalBounds().toFloat().reduced (2.f);
    g.setColour (col::panel);
    g.fillRoundedRectangle (r, 10.f);
    if (activity > 0.02f)
    {
        juce::ColourGradient glow (colour.withAlpha (0.18f * activity), r.getCentreX(), r.getY(), juce::Colours::transparentBlack,
                                   r.getCentreX(), r.getY() + r.getHeight() * 0.6f, false);
        g.setGradientFill (glow);
        g.fillRoundedRectangle (r, 10.f);
    }
    g.setColour (activity > 0.5f ? colour.withAlpha (0.8f) : col::edge);
    g.drawRoundedRectangle (r, 10.f, 1.2f);
}

void VoiceStrip::resized()
{
    auto r = getLocalBounds().reduced (6);
    enable.setBounds (r.removeFromTop (22));
    r.removeFromTop (4);
    fader.setBounds (r.removeFromLeft (46));
    r.removeFromLeft (2);
    const int rowH = r.getHeight() / 3;
    auto row1 = r.removeFromTop (rowH), row2 = r.removeFromTop (rowH), row3 = r;
    pan.setBounds (row1.removeFromLeft (row1.getWidth() / 2));
    interval.setBounds (row1);
    octave.setBounds (row2.removeFromLeft (row2.getWidth() / 2));
    semis.setBounds (row2);
    fine.setBounds (row3.removeFromLeft (row3.getWidth() / 2));
}

// ============================================================ Editor
namespace
{
constexpr int kW = 1180, kH = 790;
}

HarmonyForgeEditor::HarmonyForgeEditor (HarmonyForgeProcessor& p)
    : AudioProcessorEditor (&p), proc (p),
      key (p.apvts, pid::key), scale (p.apvts, pid::scale), mode (p.apvts, pid::mode), latency (p.apvts, pid::latency),
      robot (p.apvts, pid::robot, "ROBOT", col::accent2),
      strength (p.apvts, pid::strength, "CORRECTION", col::accent),
      speed (p.apvts, pid::speed, "RETUNE", col::accent),
      humanize (p.apvts, pid::humanize, "HUMANIZE", col::accent),
      inGain (p.apvts, pid::inGain, "INPUT", col::gold),
      outGain (p.apvts, pid::outGain, "OUTPUT", col::gold),
      mix (p.apvts, pid::mix, "DRY / WET", col::accent2),
      mainVol (p.apvts, pid::mainVol, "LEAD", col::accent),
      pitchView (p),
      meterIn (p.meterIn, col::accent), meterOut (p.meterOut, col::accent)
{
    setLookAndFeel (&laf);

    for (auto* c : std::initializer_list<juce::Component*> { &key, &scale, &mode, &latency, &robot, &strength, &speed, &humanize,
                                                              &inGain, &outGain, &mix, &mainVol, &pitchView, &meterIn, &meterOut })
        addAndMakeVisible (c);

    for (int i = 0; i < 12; ++i)
    {
        const bool black = isAllowed (i, hf::maskFrom ({ 1, 3, 6, 8, 10 }));
        noteButtons.push_back (std::make_unique<Toggle> (p.apvts, pid::note (i), kNoteNames[i], black ? col::accent2 : col::accent));
        addAndMakeVisible (*noteButtons.back());
    }

    for (int v = 0; v < hf::kNumVoices; ++v)
    {
        strips.push_back (std::make_unique<VoiceStrip> (p, v));
        addAndMakeVisible (*strips.back());
    }

    struct F { const char* id; const char* name; int group; };
    const F defs[] = {
        { pid::revSize, "SIZE", 0 }, { pid::revDamp, "DAMP", 0 }, { pid::revMix, "MIX", 0 },
        { pid::dlyTime, "TIME", 1 }, { pid::dlyFb, "FEEDBACK", 1 }, { pid::dlyMix, "MIX", 1 },
        { pid::chRate, "RATE", 2 }, { pid::chDepth, "DEPTH", 2 }, { pid::chMix, "MIX", 2 },
        { pid::satDrive, "DRIVE", 3 }, { pid::satMix, "MIX", 3 },
        { pid::eqLow, "LOW", 4 }, { pid::eqMid, "MID", 4 }, { pid::eqHigh, "HIGH", 4 } };
    const juce::Colour gc[5] = { col::accent, col::voice (3), col::voice (5), col::accent2, col::gold };
    for (const auto& d : defs)
    {
        fx.push_back (std::make_unique<Knob> (p.apvts, d.id, d.name, gc[d.group], 56));
        addAndMakeVisible (*fx.back());
    }

    for (int i = 0; i < p.getNumPrograms(); ++i) presetBox.addItem (p.getProgramName (i), i + 1);
    presetBox.setSelectedId (p.getCurrentProgram() + 1, juce::dontSendNotification);
    presetBox.onChange = [this] { proc.setCurrentProgram (presetBox.getSelectedId() - 1); lastPreset = proc.getCurrentProgram(); };
    addAndMakeVisible (presetBox);

    setResizable (true, true);
    setResizeLimits (kW * 3 / 4, kH * 3 / 4, kW * 3 / 2, kH * 3 / 2);
    getConstrainer()->setFixedAspectRatio ((double) kW / (double) kH);
    setSize (kW, kH);
    startTimerHz (30);
}

HarmonyForgeEditor::~HarmonyForgeEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void HarmonyForgeEditor::timerCallback()
{
    pitchView.tick();
    meterIn.tick();
    meterOut.tick();
    for (auto& s : strips) s->tick();
    if (lastPreset != proc.getCurrentProgram())
    {
        lastPreset = proc.getCurrentProgram();
        presetBox.setSelectedId (lastPreset + 1, juce::dontSendNotification);
    }
}

void HarmonyForgeEditor::paint (juce::Graphics& g)
{
    juce::ColourGradient bg (juce::Colour (0xff0d1020), 0.f, 0.f, col::bg, 0.f, (float) getHeight(), false);
    g.setGradientFill (bg);
    g.fillAll();

    const float s = (float) getWidth() / (float) kW;
    g.setColour (col::text);
    g.setFont (fnt (26.f * s, true));
    g.drawText ("HARMONY", (int) (22 * s), (int) (8 * s), (int) (140 * s), (int) (38 * s), juce::Justification::centredLeft);
    g.setColour (col::accent);
    g.drawText ("FORGE", (int) (150 * s), (int) (8 * s), (int) (120 * s), (int) (38 * s), juce::Justification::centredLeft);
    g.setColour (col::dim);
    g.setFont (fnt (11.f * s));
    g.drawText ("Auto-Tune + 8-voice Harmonizer", (int) (270 * s), (int) (20 * s), (int) (240 * s), (int) (16 * s), juce::Justification::centredLeft);

    auto panel = [&] (juce::Rectangle<int> r, const char* title)
    {
        g.setColour (col::panel);
        g.fillRoundedRectangle (r.toFloat(), 10.f * s);
        g.setColour (col::edge);
        g.drawRoundedRectangle (r.toFloat().reduced (0.5f), 10.f * s, 1.f);
        if (title != nullptr)
        {
            g.setColour (col::dim);
            g.setFont (fnt (10.f * s, true));
            g.drawText (title, r.getX() + (int) (10 * s), r.getY() + (int) (4 * s), r.getWidth(), (int) (14 * s), juce::Justification::centredLeft);
        }
    };
    panel (juce::Rectangle<int> ((int) (10 * s), (int) (62 * s), (int) (346 * s), (int) (300 * s)), "TUNING");
    panel (juce::Rectangle<int> ((int) (960 * s), (int) (62 * s), (int) (210 * s), (int) (300 * s)), "LEVELS");

    const char* names[5] = { "REVERB", "DELAY", "CHORUS", "SATURATION", "EQ" };
    for (int i = 0; i < 5; ++i) panel (fxGroups[i], names[i]);
}

void HarmonyForgeEditor::resized()
{
    const float s = (float) getWidth() / (float) kW;
    auto R = [&] (int x, int y, int w, int h) { return juce::Rectangle<int> ((int) (x * s), (int) (y * s), (int) (w * s), (int) (h * s)); };

    presetBox.setBounds (R (560, 14, 260, 30));
    latency.setBounds (R (840, 14, 170, 30));
    meterIn.setBounds (R (1030, 14, 10, 30));
    meterOut.setBounds (R (1046, 14, 10, 30));

    // Tuning
    key.setBounds (R (22, 84, 70, 26));
    scale.setBounds (R (98, 84, 130, 26));
    mode.setBounds (R (234, 84, 112, 26));
    strength.setBounds (R (22, 122, 100, 100));
    speed.setBounds (R (132, 122, 100, 100));
    humanize.setBounds (R (242, 122, 100, 100));
    robot.setBounds (R (22, 232, 90, 26));
    for (int i = 0; i < 12; ++i)
    {
        const int col = i % 6, row = i / 6;
        noteButtons[(size_t) i]->setBounds (R (22 + col * 54, 270 + row * 42, 50, 36));
    }
    // Pitch view
    pitchView.setBounds (R (366, 62, 584, 300));
    // Levels
    inGain.setBounds (R (972, 84, 90, 100));
    outGain.setBounds (R (1068, 84, 90, 100));
    mix.setBounds (R (972, 196, 90, 100));
    mainVol.setBounds (R (1068, 196, 90, 100));

    // Voices
    for (int v = 0; v < hf::kNumVoices; ++v)
        strips[(size_t) v]->setBounds (R (10 + v * 145, 372, 145, 268));

    // FX
    const int widths[5] = { 3, 3, 3, 2, 3 };
    const int totalKnobs = 14;
    const int usable = 1160, gap = 8;
    const int knobW = (usable - gap * 4) / totalKnobs;
    int x = 10, k = 0;
    for (int g = 0; g < 5; ++g)
    {
        const int w = widths[g] * knobW;
        fxGroups[g] = R (x, 650, w, 132);
        for (int i = 0; i < widths[g]; ++i)
            fx[(size_t) k++]->setBounds (R (x + i * knobW, 666, knobW, 112));
        x += w + gap;
    }
}
