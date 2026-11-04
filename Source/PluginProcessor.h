#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "Parameters.h"
#include "Presets.h"
#include "HarmonyEngine.h"
#include "StereoDelay.h"

class HarmonyForgeProcessor : public juce::AudioProcessor
{
public:
    HarmonyForgeProcessor();
    ~HarmonyForgeProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "HarmonyForge"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 3.0; }

    int getNumPrograms() override { return (int) hf::presets().size(); }
    int getCurrentProgram() override { return currentPreset; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& dest) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    uint16_t currentScaleMask() const;

    juce::AudioProcessorValueTreeState apvts;
    hf::HarmonyEngine engine;
    std::atomic<float> meterIn { 0.f }, meterOut { 0.f };

private:
    void updateParameters();
    void processFx (int n);
    void updateEq();

    struct Raw
    {
        std::atomic<float> *key, *scale, *strength, *speed, *humanize, *robot, *mode, *mix, *inGain, *outGain, *latency, *mainVol;
        std::atomic<float> *revSize, *revDamp, *revMix, *dlyTime, *dlyFb, *dlyMix, *chRate, *chDepth, *chMix;
        std::atomic<float> *satDrive, *satMix, *eqLow, *eqMid, *eqHigh;
        std::atomic<float> *note[12];
        struct V { std::atomic<float> *on, *vol, *deg, *oct, *semi, *cents, *pan; } v[hf::kNumVoices];
    } raw {};

    struct DryDelay
    {
        std::vector<float> buf; int w = 0;
        void prepare (int maxSamples) { buf.assign ((size_t) maxSamples + 2, 0.f); w = 0; }
        float process (float x, int delay)
        {
            const int n = (int) buf.size();
            buf[(size_t) w] = x;
            int r = w - delay; if (r < 0) r += n;
            if (++w >= n) w = 0;
            return buf[(size_t) r];
        }
    } dryDelay[2];

    using Smooth = juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear>;
    Smooth inGain, outGain, mixSm, mainGain, voiceL[hf::kNumVoices], voiceR[hf::kNumVoices];

    juce::AudioBuffer<float> wetBuffer, dryBuffer;
    juce::dsp::Chorus<float> chorus;
    juce::Reverb reverb;
    hf::StereoDelay delay;
    juce::dsp::IIR::Filter<float> eq[3][2];
    float eqCache[3] { 1e9f, 1e9f, 1e9f };

    double sr = 48000.0;
    int currentLatencyMode = -1, dryDelaySamples = 0, currentPreset = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HarmonyForgeProcessor)
};
