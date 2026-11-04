#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "Scale.h"
#include "HarmonyEngine.h"

namespace hf::pid
{
inline constexpr const char* key = "key";
inline constexpr const char* scale = "scale";
inline constexpr const char* strength = "strength";
inline constexpr const char* speed = "speed";
inline constexpr const char* humanize = "humanize";
inline constexpr const char* robot = "robot";
inline constexpr const char* mode = "mode";
inline constexpr const char* mix = "mix";
inline constexpr const char* inGain = "in_gain";
inline constexpr const char* outGain = "out_gain";
inline constexpr const char* latency = "latency";
inline constexpr const char* mainVol = "main_vol";
inline constexpr const char* revSize = "rev_size";
inline constexpr const char* revDamp = "rev_damp";
inline constexpr const char* revMix = "rev_mix";
inline constexpr const char* dlyTime = "dly_time";
inline constexpr const char* dlyFb = "dly_fb";
inline constexpr const char* dlyMix = "dly_mix";
inline constexpr const char* chRate = "ch_rate";
inline constexpr const char* chDepth = "ch_depth";
inline constexpr const char* chMix = "ch_mix";
inline constexpr const char* satDrive = "sat_drive";
inline constexpr const char* satMix = "sat_mix";
inline constexpr const char* eqLow = "eq_low";
inline constexpr const char* eqMid = "eq_mid";
inline constexpr const char* eqHigh = "eq_high";

inline juce::String note (int n) { return "note_" + juce::String (n); }
inline juce::String voice (int v, const char* s) { return "v" + juce::String (v + 1) + "_" + s; }

inline constexpr const char* vEnable = "on";
inline constexpr const char* vVol = "vol";
inline constexpr const char* vDeg = "deg";
inline constexpr const char* vOct = "oct";
inline constexpr const char* vSemi = "semi";
inline constexpr const char* vCents = "cents";
inline constexpr const char* vPan = "pan";
} // namespace hf::pid

namespace hf
{
inline juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
{
    using APF = juce::AudioParameterFloat;
    using APB = juce::AudioParameterBool;
    using APC = juce::AudioParameterChoice;
    using API = juce::AudioParameterInt;
    using Attr = juce::AudioParameterFloatAttributes;
    using juce::NormalisableRange;
    using juce::ParameterID;

    juce::AudioProcessorValueTreeState::ParameterLayout L;

    auto flt = [&] (const juce::String& id, const juce::String& name, float lo, float hi, float def,
                    const juce::String& unit = {}, float skew = 1.f, float step = 0.01f)
    {
        L.add (std::make_unique<APF> (ParameterID { id, 1 }, name, NormalisableRange<float> (lo, hi, step, skew), def,
                                      Attr().withLabel (unit)));
    };

    juce::StringArray keys, scales;
    for (auto* n : kNoteNames) keys.add (n);
    for (auto* n : kScaleNames) scales.add (n);

    L.add (std::make_unique<APC> (ParameterID { pid::key, 1 }, "Key", keys, 0));
    L.add (std::make_unique<APC> (ParameterID { pid::scale, 1 }, "Scale", scales, (int) Major));
    for (int i = 0; i < 12; ++i)
        L.add (std::make_unique<APB> (ParameterID { pid::note (i), 1 }, juce::String ("Note ") + kNoteNames[i], true));

    flt (pid::strength, "Correction", 0.f, 100.f, 100.f, "%", 1.f, 0.1f);
    flt (pid::speed, "Retune Speed", 0.f, 200.f, 25.f, "ms", 0.4f, 0.1f);
    flt (pid::humanize, "Humanize", 0.f, 100.f, 20.f, "%", 1.f, 0.1f);
    L.add (std::make_unique<APB> (ParameterID { pid::robot, 1 }, "Robot", false));
    L.add (std::make_unique<APC> (ParameterID { pid::mode, 1 }, "Harmony Mode", juce::StringArray { "Auto (Scale)", "MIDI" }, 0));
    flt (pid::mix, "Dry/Wet", 0.f, 100.f, 100.f, "%", 1.f, 0.1f);
    flt (pid::inGain, "Input", -24.f, 24.f, 0.f, "dB", 1.f, 0.1f);
    flt (pid::outGain, "Output", -24.f, 24.f, 0.f, "dB", 1.f, 0.1f);
    L.add (std::make_unique<APC> (ParameterID { pid::latency, 1 }, "Latency",
                                  juce::StringArray { "Low (14 ms)", "Normal (24 ms)", "Smooth (36 ms)" }, 1));
    flt (pid::mainVol, "Lead Level", -60.f, 6.f, 0.f, "dB", 1.f, 0.1f);

    flt (pid::revSize, "Reverb Size", 0.f, 100.f, 40.f, "%", 1.f, 0.1f);
    flt (pid::revDamp, "Reverb Damp", 0.f, 100.f, 50.f, "%", 1.f, 0.1f);
    flt (pid::revMix, "Reverb Mix", 0.f, 100.f, 0.f, "%", 1.f, 0.1f);
    flt (pid::dlyTime, "Delay Time", 1.f, 1000.f, 300.f, "ms", 0.5f, 1.f);
    flt (pid::dlyFb, "Delay Feedback", 0.f, 90.f, 35.f, "%", 1.f, 0.1f);
    flt (pid::dlyMix, "Delay Mix", 0.f, 100.f, 0.f, "%", 1.f, 0.1f);
    flt (pid::chRate, "Chorus Rate", 0.1f, 8.f, 0.8f, "Hz", 0.5f, 0.01f);
    flt (pid::chDepth, "Chorus Depth", 0.f, 100.f, 40.f, "%", 1.f, 0.1f);
    flt (pid::chMix, "Chorus Mix", 0.f, 100.f, 0.f, "%", 1.f, 0.1f);
    flt (pid::satDrive, "Saturation Drive", 0.f, 100.f, 20.f, "%", 1.f, 0.1f);
    flt (pid::satMix, "Saturation Mix", 0.f, 100.f, 0.f, "%", 1.f, 0.1f);
    flt (pid::eqLow, "EQ Low", -12.f, 12.f, 0.f, "dB", 1.f, 0.1f);
    flt (pid::eqMid, "EQ Mid", -12.f, 12.f, 0.f, "dB", 1.f, 0.1f);
    flt (pid::eqHigh, "EQ High", -12.f, 12.f, 0.f, "dB", 1.f, 0.1f);

    const int defDeg[8] = { 2, 4, -2, -3, 7, -7, 1, -1 };
    const float defPan[8] = { -0.5f, 0.5f, -0.8f, 0.8f, -0.25f, 0.25f, -1.f, 1.f };
    for (int v = 0; v < kNumVoices; ++v)
    {
        const juce::String n = "Voice " + juce::String (v + 1) + " ";
        L.add (std::make_unique<APB> (ParameterID { pid::voice (v, pid::vEnable), 1 }, n + "On", false));
        flt (pid::voice (v, pid::vVol), n + "Level", -60.f, 6.f, -4.f, "dB", 1.f, 0.1f);
        L.add (std::make_unique<API> (ParameterID { pid::voice (v, pid::vDeg), 1 }, n + "Interval", -7, 7, defDeg[v]));
        L.add (std::make_unique<API> (ParameterID { pid::voice (v, pid::vOct), 1 }, n + "Octave", -2, 2, 0));
        L.add (std::make_unique<API> (ParameterID { pid::voice (v, pid::vSemi), 1 }, n + "Semitones", -12, 12, 0));
        flt (pid::voice (v, pid::vCents), n + "Fine", -100.f, 100.f, 0.f, "ct", 1.f, 0.1f);
        flt (pid::voice (v, pid::vPan), n + "Pan", -1.f, 1.f, defPan[v], {}, 1.f, 0.01f);
    }
    return L;
}
} // namespace hf
