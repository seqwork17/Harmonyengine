#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace hf;

HarmonyForgeProcessor::HarmonyForgeProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "HarmonyForgeState", createLayout())
{
    auto g = [this] (const juce::String& id) { return apvts.getRawParameterValue (id); };

    raw.key = g (pid::key);         raw.scale = g (pid::scale);       raw.strength = g (pid::strength);
    raw.speed = g (pid::speed);     raw.humanize = g (pid::humanize); raw.robot = g (pid::robot);
    raw.mode = g (pid::mode);       raw.mix = g (pid::mix);           raw.inGain = g (pid::inGain);
    raw.outGain = g (pid::outGain); raw.latency = g (pid::latency);   raw.mainVol = g (pid::mainVol);
    raw.revSize = g (pid::revSize); raw.revDamp = g (pid::revDamp);   raw.revMix = g (pid::revMix);
    raw.dlyTime = g (pid::dlyTime); raw.dlyFb = g (pid::dlyFb);       raw.dlyMix = g (pid::dlyMix);
    raw.chRate = g (pid::chRate);   raw.chDepth = g (pid::chDepth);   raw.chMix = g (pid::chMix);
    raw.satDrive = g (pid::satDrive); raw.satMix = g (pid::satMix);
    raw.eqLow = g (pid::eqLow);     raw.eqMid = g (pid::eqMid);       raw.eqHigh = g (pid::eqHigh);
    for (int i = 0; i < 12; ++i) raw.note[i] = g (pid::note (i));
    for (int v = 0; v < kNumVoices; ++v)
    {
        raw.v[v].on = g (pid::voice (v, pid::vEnable));  raw.v[v].vol = g (pid::voice (v, pid::vVol));
        raw.v[v].deg = g (pid::voice (v, pid::vDeg));    raw.v[v].oct = g (pid::voice (v, pid::vOct));
        raw.v[v].semi = g (pid::voice (v, pid::vSemi));  raw.v[v].cents = g (pid::voice (v, pid::vCents));
        raw.v[v].pan = g (pid::voice (v, pid::vPan));
    }
}

bool HarmonyForgeProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto& o = l.getMainOutputChannelSet();
    const auto& i = l.getMainInputChannelSet();
    const bool outOk = o == juce::AudioChannelSet::mono() || o == juce::AudioChannelSet::stereo();
    const bool inOk = i == juce::AudioChannelSet::mono() || i == juce::AudioChannelSet::stereo();
    return outOk && inOk;
}

void HarmonyForgeProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sr = sampleRate;
    engine.prepare (sampleRate);
    currentLatencyMode = -1;

    const int maxBlock = std::max (samplesPerBlock, 64);
    wetBuffer.setSize (2, maxBlock);
    dryBuffer.setSize (2, maxBlock);
    wetBuffer.clear(); dryBuffer.clear();

    const int maxDry = (int) (sampleRate * 0.06) + 8;
    dryDelay[0].prepare (maxDry);
    dryDelay[1].prepare (maxDry);

    juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) maxBlock, 2 };
    chorus.prepare (spec);
    chorus.setCentreDelay (7.0f);
    chorus.setFeedback (0.f);
    reverb.setSampleRate (sampleRate);
    reverb.reset();
    delay.prepare (sampleRate);

    for (auto& band : eq) for (auto& f : band) f.reset();
    for (float& c : eqCache) c = 1e9f;
    updateEq();

    auto init = [&] (Smooth& s, float v, double secs = 0.02) { s.reset (sampleRate, secs); s.setCurrentAndTargetValue (v); };
    init (inGain, 1.f); init (outGain, 1.f); init (mixSm, 1.f); init (mainGain, 1.f);
    for (int v = 0; v < kNumVoices; ++v) { init (voiceL[v], 0.f); init (voiceR[v], 0.f); }

    updateParameters();
}

static float dbToGain (float db) { return db <= -59.9f ? 0.f : juce::Decibels::decibelsToGain (db); }

uint16_t HarmonyForgeProcessor::currentScaleMask() const
{
    uint16_t custom = 0;
    for (int i = 0; i < 12; ++i) if (raw.note[i]->load() > 0.5f) custom = (uint16_t) (custom | (1u << i));
    return buildMask ((int) raw.key->load(), (int) raw.scale->load(), custom);
}

void HarmonyForgeProcessor::updateEq()
{
    const float gains[3] = { raw.eqLow->load(), raw.eqMid->load(), raw.eqHigh->load() };
    for (int b = 0; b < 3; ++b)
    {
        if (std::abs (gains[b] - eqCache[b]) < 0.001f) continue;
        eqCache[b] = gains[b];
        const float lin = juce::Decibels::decibelsToGain (gains[b]);
        juce::dsp::IIR::Coefficients<float>::Ptr c;
        if (b == 0)      c = juce::dsp::IIR::Coefficients<float>::makeLowShelf (sr, 120.0, 0.707f, lin);
        else if (b == 1) c = juce::dsp::IIR::Coefficients<float>::makePeakFilter (sr, 1500.0, 0.8f, lin);
        else             c = juce::dsp::IIR::Coefficients<float>::makeHighShelf (sr, 6500.0, 0.707f, lin);
        eq[b][0].coefficients = c;
        eq[b][1].coefficients = c;
    }
}

void HarmonyForgeProcessor::updateParameters()
{
    EngineParams ep;
    ep.key = (int) raw.key->load();
    ep.scaleType = (int) raw.scale->load();
    uint16_t custom = 0;
    for (int i = 0; i < 12; ++i) if (raw.note[i]->load() > 0.5f) custom = (uint16_t) (custom | (1u << i));
    ep.customMask = custom;
    ep.strength = raw.strength->load() * 0.01f;
    ep.speedMs = raw.speed->load();
    ep.humanize = raw.humanize->load() * 0.01f;
    ep.robot = raw.robot->load() > 0.5f;
    ep.midiMode = (int) raw.mode->load() == 1;
    for (int v = 0; v < kNumVoices; ++v)
    {
        auto& d = ep.voices[v];
        d.enabled = raw.v[v].on->load() > 0.5f;
        d.degrees = (int) std::lround (raw.v[v].deg->load());
        d.octave = (int) std::lround (raw.v[v].oct->load());
        d.semis = (int) std::lround (raw.v[v].semi->load());
        d.cents = raw.v[v].cents->load();
    }
    engine.setParams (ep);

    const int lat = (int) raw.latency->load();
    if (lat != currentLatencyMode)
    {
        currentLatencyMode = lat;
        const float ms = lat == 0 ? 14.f : lat == 1 ? 24.f : 36.f;
        engine.setWindowMs (ms);
        dryDelaySamples = engine.latencySamples();
        setLatencySamples (dryDelaySamples);
    }

    inGain.setTargetValue (dbToGain (raw.inGain->load()));
    outGain.setTargetValue (dbToGain (raw.outGain->load()));
    mixSm.setTargetValue (raw.mix->load() * 0.01f);
    mainGain.setTargetValue (dbToGain (raw.mainVol->load()));

    for (int v = 0; v < kNumVoices; ++v)
    {
        const float g = dbToGain (raw.v[v].vol->load());
        const float pan = (raw.v[v].pan->load() + 1.f) * 0.25f * juce::MathConstants<float>::pi; // 0..pi/2
        voiceL[v].setTargetValue (g * std::cos (pan));
        voiceR[v].setTargetValue (g * std::sin (pan));
    }

    updateEq();
    chorus.setRate (raw.chRate->load());
    chorus.setDepth (raw.chDepth->load() * 0.01f);
    chorus.setMix (raw.chMix->load() * 0.01f);

    juce::Reverb::Parameters rp;
    rp.roomSize = raw.revSize->load() * 0.01f;
    rp.damping = raw.revDamp->load() * 0.01f;
    rp.wetLevel = raw.revMix->load() * 0.01f * 0.33f;
    rp.dryLevel = 1.f;
    rp.width = 1.f;
    reverb.setParameters (rp);

    delay.setParams (raw.dlyTime->load(), raw.dlyFb->load() * 0.01f, raw.dlyMix->load() * 0.01f);
}

void HarmonyForgeProcessor::processFx (int n)
{
    float* wl = wetBuffer.getWritePointer (0);
    float* wr = wetBuffer.getWritePointer (1);

    // EQ
    if (eqCache[0] != 0.f || eqCache[1] != 0.f || eqCache[2] != 0.f)
        for (int i = 0; i < n; ++i)
            for (int b = 0; b < 3; ++b)
            {
                wl[i] = eq[b][0].processSample (wl[i]);
                wr[i] = eq[b][1].processSample (wr[i]);
            }

    // Chorus
    if (raw.chMix->load() > 0.05f)
    {
        float* ch[2] = { wl, wr };
        juce::dsp::AudioBlock<float> block (ch, 2, (size_t) n);
        juce::dsp::ProcessContextReplacing<float> ctx (block);
        chorus.process (ctx);
    }

    // Saturación
    const float sMix = raw.satMix->load() * 0.01f;
    if (sMix > 0.0005f)
    {
        const float x = raw.satDrive->load() * 0.01f;
        const float drive = 1.f + 19.f * x * x;
        const float norm = 1.f / std::tanh (drive);
        for (int i = 0; i < n; ++i)
        {
            wl[i] += (std::tanh (wl[i] * drive) * norm - wl[i]) * sMix;
            wr[i] += (std::tanh (wr[i] * drive) * norm - wr[i]) * sMix;
        }
    }

    if (delay.active()) delay.process (wl, wr, n);
    if (raw.revMix->load() > 0.05f) reverb.processStereo (wl, wr, n);
}

void HarmonyForgeProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int nIn = getTotalNumInputChannels();
    const int nOut = getTotalNumOutputChannels();
    const int n = buffer.getNumSamples();
    if (n == 0) return;

    updateParameters();

    for (const auto meta : midi)
    {
        const auto m = meta.getMessage();
        if (m.isNoteOn()) engine.noteOn (m.getNoteNumber());
        else if (m.isNoteOff()) engine.noteOff (m.getNoteNumber());
        else if (m.isAllNotesOff() || m.isAllSoundOff()) engine.allNotesOff();
    }

    if (wetBuffer.getNumSamples() < n)
    {
        wetBuffer.setSize (2, n, false, false, true);
        dryBuffer.setSize (2, n, false, false, true);
    }

    float* inL = buffer.getWritePointer (0);
    float* inR = nIn > 1 ? buffer.getWritePointer (1) : nullptr;
    float* wl = wetBuffer.getWritePointer (0);
    float* wr = wetBuffer.getWritePointer (1);
    float* dl = dryBuffer.getWritePointer (0);
    float* dr = dryBuffer.getWritePointer (1);

    float v[kNumVoices + 1];
    float peakIn = 0.f;

    for (int i = 0; i < n; ++i)
    {
        const float g = inGain.getNextValue();
        const float l = inL[i] * g;
        const float r = inR != nullptr ? inR[i] * g : l;
        peakIn = std::max (peakIn, std::max (std::abs (l), std::abs (r)));

        engine.process (0.5f * (l + r), v);

        const float lead = v[0] * mainGain.getNextValue();
        float sl = lead, sr2 = lead;
        for (int k = 0; k < kNumVoices; ++k)
        {
            sl += v[k + 1] * voiceL[k].getNextValue();
            sr2 += v[k + 1] * voiceR[k].getNextValue();
        }
        wl[i] = sl;
        wr[i] = sr2;
        dl[i] = dryDelay[0].process (l, dryDelaySamples);
        dr[i] = dryDelay[1].process (r, dryDelaySamples);
    }

    processFx (n);

    float* outL = buffer.getWritePointer (0);
    float* outR = nOut > 1 ? buffer.getWritePointer (1) : nullptr;
    float peakOut = 0.f;
    for (int i = 0; i < n; ++i)
    {
        const float m = mixSm.getNextValue();
        const float og = outGain.getNextValue();
        const float l = (dl[i] * (1.f - m) + wl[i] * m) * og;
        const float r = (dr[i] * (1.f - m) + wr[i] * m) * og;
        if (outR != nullptr) { outL[i] = l; outR[i] = r; }
        else outL[i] = 0.5f * (l + r);
        peakOut = std::max (peakOut, std::max (std::abs (l), std::abs (r)));
    }

    const float decay = std::pow (0.9f, (float) n / 512.f);
    meterIn.store (std::max (peakIn, meterIn.load() * decay));
    meterOut.store (std::max (peakOut, meterOut.load() * decay));
}

// ---- Estado y presets -------------------------------------------------------
void HarmonyForgeProcessor::setCurrentProgram (int index)
{
    const auto& list = presets();
    if (index < 0 || index >= (int) list.size()) return;
    currentPreset = index;

    for (auto* p : getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
            rp->setValueNotifyingHost (rp->getDefaultValue());

    for (const auto& kv : list[(size_t) index].values)
        if (auto* p = apvts.getParameter (kv.first))
            p->setValueNotifyingHost (p->convertTo0to1 (kv.second));
}

const juce::String HarmonyForgeProcessor::getProgramName (int index)
{
    const auto& list = presets();
    return (index >= 0 && index < (int) list.size()) ? juce::String (list[(size_t) index].name) : juce::String();
}

void HarmonyForgeProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    auto state = apvts.copyState();
    state.setProperty ("preset", currentPreset, nullptr);
    if (auto xml = state.createXml()) copyXmlToBinary (*xml, dest);
}

void HarmonyForgeProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto tree = juce::ValueTree::fromXml (*xml);
            currentPreset = juce::jlimit (0, (int) presets().size() - 1, (int) tree.getProperty ("preset", 0));
            apvts.replaceState (tree);
        }
}

juce::AudioProcessorEditor* HarmonyForgeProcessor::createEditor() { return new HarmonyForgeEditor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new HarmonyForgeProcessor(); }
