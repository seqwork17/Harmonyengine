#pragma once
#include "Scale.h"
#include "YinDetector.h"
#include "MultiShifter.h"
#include <atomic>
#include <vector>
#include <cmath>
#include <algorithm>
#include <cstdint>

namespace hf
{
constexpr int kNumVoices = 8;

struct VoiceParams
{
    bool enabled = false;
    int degrees = 0;   // grados de escala (modo Auto)
    int octave = 0;
    int semis = 0;
    float cents = 0.f;
};

struct EngineParams
{
    int key = 0;
    int scaleType = Major;
    uint16_t customMask = 0x0FFF;
    float strength = 1.f;   // 0..1
    float speedMs = 25.f;   // tiempo de retune
    float humanize = 0.2f;  // 0..1
    bool robot = false;
    bool midiMode = false;
    VoiceParams voices[kNumVoices];
};

class HarmonyEngine
{
public:
    HarmonyEngine()
    {
        for (int i = 0; i < kNumVoices; ++i) { telVoice[i].store (-1.f); telVoiceOn[i].store (0.f); }
    }

    void prepare (double sampleRate)
    {
        fs = sampleRate;
        D = std::max (1, (int) std::lround (sampleRate / 16000.0));
        yin.prepare (sampleRate / D, 65.f, 1000.f);
        frame.assign ((size_t) yin.frameSize(), 0.f);
        dring.assign (4096, 0.f);
        dmask = 4095;
        dHop = std::max (8, 128 / D);
        shifter.prepare (sampleRate, 60.f);
        gAlpha = 1.f - std::exp (-1.f / (0.005f * (float) fs));
        setWindowMs (windowMs);
        reset();
        setParams (params);
    }

    void reset()
    {
        std::fill (dring.begin(), dring.end(), 0.f);
        dpos = 0; decAcc = 0.f; decCount = 0; hopCount = 0;
        for (int i = 0; i <= kNumVoices; ++i) { cur[i] = 0.f; tgt[i] = 0.f; vGain[i] = 0.f; }
        for (int i = 0; i < kNumVoices; ++i) { wantOn[i] = false; justActivated[i] = false; drift[i] = 0.f; }
        for (int i = 0; i < 3; ++i) medianHist[i] = -1.f;
        heldCount = 0;
        periodSmooth = 0.f;
        shifter.reset();
        rng = 0x9E3779B9u;
    }

    void setWindowMs (float ms)
    {
        windowMs = ms;
        windowSamples = (float) (ms * 0.001 * fs);
        shifter.setWindowSamples (windowSamples);
        latency = (int) std::lround (windowSamples * 0.5f) + 1;
    }

    int latencySamples() const { return latency; }

    void setParams (const EngineParams& p)
    {
        params = p;
        const float hum = p.robot ? 0.f : p.humanize;
        const float tauMs = p.robot ? 0.f : p.speedMs * (1.f + 3.f * hum);
        alpha = tauMs < 0.05f ? 1.f : 1.f - std::exp (-1000.f / ((float) fs * tauMs));
        refreshActive();
    }

    void noteOn (int n)
    {
        for (int i = 0; i < heldCount; ++i) if (held[i] == n) return;
        if (heldCount < kNumVoices) held[heldCount++] = n;
        refreshActive();
    }

    void noteOff (int n)
    {
        for (int i = 0; i < heldCount; ++i)
            if (held[i] == n)
            {
                for (int j = i; j < heldCount - 1; ++j) held[j] = held[j + 1];
                --heldCount;
                break;
            }
        refreshActive();
    }

    void allNotesOff() { heldCount = 0; refreshActive(); }

    // out[0] = voz principal corregida, out[1..8] = voces armónicas (mono, sin pan ni volumen).
    void process (float x, float* out)
    {
        decAcc += x;
        if (++decCount >= D)
        {
            dring[(size_t) (dpos & dmask)] = decAcc / (float) D;
            ++dpos; decAcc = 0.f; decCount = 0;
            if (++hopCount >= dHop) { hopCount = 0; analyse(); }
        }

        shifter.push (x);

        cur[0] += (tgt[0] - cur[0]) * alpha;
        out[0] = shifter.read (0, std::exp2 (cur[0]));

        for (int v = 0; v < kNumVoices; ++v)
        {
            const float target = wantOn[v] ? 1.f : 0.f;
            float g = vGain[v + 1];
            g += (target - g) * gAlpha;
            if (target == 0.f && g < 1e-4f) { g = 0.f; vGain[v + 1] = 0.f; out[v + 1] = 0.f; continue; }
            vGain[v + 1] = g;
            cur[v + 1] += (tgt[v + 1] - cur[v + 1]) * alpha;
            out[v + 1] = shifter.read (v + 1, std::exp2 (cur[v + 1])) * g;
        }
    }

    // Telemetría para la interfaz (lectura desde el hilo de UI).
    std::atomic<float> telDetected { -1.f }, telTarget { -1.f }, telConfidence { 0.f };
    std::atomic<float> telVoice[kNumVoices];
    std::atomic<float> telVoiceOn[kNumVoices];

private:
    void refreshActive()
    {
        for (int i = 0; i < kNumVoices; ++i)
        {
            const bool want = params.voices[i].enabled && (! params.midiMode || i < heldCount);
            if (want && ! wantOn[i]) justActivated[i] = true;
            wantOn[i] = want;
            telVoiceOn[i].store (want ? 1.f : 0.f);
        }
    }

    float rand11()
    {
        rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5;
        return (float) (rng & 0xFFFFu) / 32768.f - 1.f;
    }

    static float median3 (float a, float b, float c) { return std::max (std::min (a, b), std::min (std::max (a, b), c)); }

    void analyse()
    {
        const int n = (int) frame.size();
        float e = 0.f;
        for (int i = 0; i < n; ++i)
        {
            const float s = dring[(size_t) ((dpos - n + i) & dmask)];
            frame[(size_t) i] = s;
            e += s * s;
        }
        const float rms = std::sqrt (e / (float) n);

        float f = 0.f, conf = 0.f;
        const bool voiced = rms > 0.0025f && yin.detect (frame.data(), f, conf);
        telConfidence.store (voiced ? conf : 0.f);
        if (! voiced) { telDetected.store (-1.f); return; }   // mantiene el último ratio

        const float T0 = (float) fs / f;
        periodSmooth = periodSmooth > 0.f ? periodSmooth + (T0 - periodSmooth) * 0.25f : T0;
        shifter.setPeriod (periodSmooth);

        float m = 69.f + 12.f * std::log2 (f / 440.f);
        medianHist[2] = medianHist[1]; medianHist[1] = medianHist[0]; medianHist[0] = m;
        if (medianHist[2] > 0.f) m = median3 (medianHist[0], medianHist[1], medianHist[2]);

        const uint16_t mask = buildMask (params.key, params.scaleType, params.customMask);
        const float q = quantize (m, mask);
        const float err = q - m;
        const float s = params.robot ? 1.f : params.strength;
        const float hum = params.robot ? 0.f : params.humanize;
        // Humanize: deja pasar parte de la desviación natural en errores pequeños.
        const float keep = hum * (1.f - std::min (1.f, std::fabs (err) / 0.5f)) * 0.5f;
        const float corr = err * s * (1.f - keep);
        const float mainT = m + corr;
        tgt[0] = clampOct (corr / 12.f);
        const int qn = (int) std::lround (q);

        for (int v = 0; v < kNumVoices; ++v)
        {
            const VoiceParams& vp = params.voices[v];
            if (! vp.enabled) continue;
            float vt;
            if (params.midiMode)
            {
                if (v >= heldCount) continue;
                vt = (float) held[v];
            }
            else
                vt = mainT + (float) (shiftDegrees (qn, vp.degrees, mask) - qn);

            drift[v] += (rand11() - drift[v]) * 0.01f;
            vt += (float) (vp.octave * 12 + vp.semis) + vp.cents * 0.01f + drift[v] * 0.12f * hum;

            tgt[v + 1] = clampOct ((vt - m) / 12.f);
            if (justActivated[v]) { cur[v + 1] = tgt[v + 1]; justActivated[v] = false; }
            telVoice[v].store (vt);
        }
        telDetected.store (m);
        telTarget.store (mainT);
    }

    static float clampOct (float x) { return std::max (-2.f, std::min (2.f, x)); }

    double fs = 48000.0;
    int D = 3, dHop = 42, dmask = 4095, dpos = 0, decCount = 0, hopCount = 0;
    float decAcc = 0.f;
    std::vector<float> dring, frame;
    YinDetector yin;
    MultiShifter<kNumVoices + 1> shifter;

    EngineParams params;
    float windowMs = 24.f, windowSamples = 1152.f;
    int latency = 577;
    float alpha = 1.f, gAlpha = 0.01f;
    float cur[kNumVoices + 1] {}, tgt[kNumVoices + 1] {}, vGain[kNumVoices + 1] {};
    bool wantOn[kNumVoices] {}, justActivated[kNumVoices] {};
    float drift[kNumVoices] {};
    float medianHist[3] { -1.f, -1.f, -1.f };
    float periodSmooth = 0.f;
    int held[kNumVoices] {}, heldCount = 0;
    uint32_t rng = 0x9E3779B9u;
};
} // namespace hf
