#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include "PluginProcessor.h"

namespace hf::ui
{
namespace col
{
    inline const juce::Colour bg { 0xff080a11 }, panel { 0xff121626 }, panel2 { 0xff1a2036 }, edge { 0xff262e4d },
        accent { 0xff39e6ff }, accent2 { 0xffff3d9a }, text { 0xffe9eefc }, dim { 0xff7c86a8 }, gold { 0xffffc857 };
    inline juce::Colour voice (int i) { return juce::Colour::fromHSV ((float) i / 8.f * 0.92f + 0.02f, 0.62f, 1.f, 1.f); }
}

class LookAndFeel : public juce::LookAndFeel_V4
{
public:
    LookAndFeel();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos, float start, float end, juce::Slider&) override;
    void drawLinearSlider (juce::Graphics&, int x, int y, int w, int h, float pos, float minPos, float maxPos,
                           juce::Slider::SliderStyle, juce::Slider&) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool highlighted, bool down) override;
    void drawComboBox (juce::Graphics&, int w, int h, bool down, int, int, int, int, juce::ComboBox&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override { return juce::Font (juce::FontOptions (13.f)); }
};

// Mando giratorio con etiqueta y adjunto al parámetro.
class Knob : public juce::Component
{
public:
    Knob (juce::AudioProcessorValueTreeState& s, const juce::String& id, const juce::String& name, juce::Colour c, int textW = 64);
    void resized() override;
private:
    juce::Slider slider;
    juce::Label label;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att;
};

class Selector : public juce::Component
{
public:
    Selector (juce::AudioProcessorValueTreeState& s, const juce::String& id);
    void resized() override { box.setBounds (getLocalBounds()); }
private:
    juce::ComboBox box;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> att;
};

class Toggle : public juce::Component
{
public:
    Toggle (juce::AudioProcessorValueTreeState& s, const juce::String& id, const juce::String& text, juce::Colour c);
    void resized() override { btn.setBounds (getLocalBounds()); }
private:
    juce::ToggleButton btn;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> att;
};

class MeterBar : public juce::Component
{
public:
    MeterBar (std::atomic<float>& src, juce::Colour c) : source (src), colour (c) {}
    void tick();
    void paint (juce::Graphics&) override;
private:
    std::atomic<float>& source;
    juce::Colour colour;
    float level = 0.f;
};

// Visor en tiempo real de la nota detectada, la corregida y las voces armónicas.
class PitchView : public juce::Component
{
public:
    explicit PitchView (HarmonyForgeProcessor& p);
    void tick();
    void paint (juce::Graphics&) override;
private:
    static constexpr int N = 300;
    HarmonyForgeProcessor& proc;
    float detected[N], target[N], voices[hf::kNumVoices][N];
    bool voiceOn[hf::kNumVoices][N];
    int head = 0;
    float viewCentre = 60.f;
};

class VoiceStrip : public juce::Component
{
public:
    VoiceStrip (HarmonyForgeProcessor& p, int index);
    void paint (juce::Graphics&) override;
    void resized() override;
    void tick();
private:
    HarmonyForgeProcessor& proc;
    int idx;
    juce::Colour colour;
    float activity = 0.f;
    Toggle enable;
    juce::Slider fader;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> faderAtt;
    Knob pan, interval, octave, semis, fine;
};
} // namespace hf::ui

class HarmonyForgeEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit HarmonyForgeEditor (HarmonyForgeProcessor&);
    ~HarmonyForgeEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    HarmonyForgeProcessor& proc;
    hf::ui::LookAndFeel laf;

    juce::ComboBox presetBox;
    hf::ui::Selector key, scale, mode, latency;
    hf::ui::Toggle robot;
    hf::ui::Knob strength, speed, humanize, inGain, outGain, mix, mainVol;
    std::vector<std::unique_ptr<hf::ui::Toggle>> noteButtons;
    hf::ui::PitchView pitchView;
    hf::ui::MeterBar meterIn, meterOut;
    std::vector<std::unique_ptr<hf::ui::VoiceStrip>> strips;
    std::vector<std::unique_ptr<hf::ui::Knob>> fx;
    juce::Rectangle<int> fxGroups[5];
    int lastPreset = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HarmonyForgeEditor)
};
