#pragma once
#include <vector>
#include <cmath>

namespace hf
{
// Un único buffer circular compartido y N "cabezales" de pitch shifting independientes.
// Cada cabezal usa dos taps con ventana seno^2 desfasados medio ciclo (crossfade sin clicks).
template <int NumHeads>
class MultiShifter
{
public:
    void prepare (double sampleRate, float maxWindowMs)
    {
        int n = 1;
        while ((double) n < sampleRate * (double) maxWindowMs * 0.001 * 2.0 + 16.0) n <<= 1;
        buf.assign ((size_t) n, 0.f);
        mask = n - 1;
        reset();
    }

    void reset()
    {
        std::fill (buf.begin(), buf.end(), 0.f);
        wpos = 0;
        for (int i = 0; i < NumHeads; ++i) phase[i] = 0.f;
    }

    void setWindowSamples (float w) { W = w < 32.f ? 32.f : w; updateDeltas(); }

    // Periodo de la señal de entrada en muestras (0 = desconocido). Con él, la separación entre los
    // dos taps se ajusta a un múltiplo exacto del periodo para que sumen siempre en fase.
    void setPeriod (float samples) { period = samples; updateDeltas(); }

    void push (float x)
    {
        buf[(size_t) wpos] = x;
        wpos = (wpos + 1) & mask;
    }

    // ratio > 1 sube el tono, < 1 lo baja.
    float read (int head, float ratio)
    {
        float p = phase[head] + (1.f - ratio) / W;
        p -= std::floor (p);
        phase[head] = p;
        float p2 = p + 0.5f;
        float extra = deltaPlus;
        if (p2 >= 1.f) { p2 -= 1.f; extra = deltaMinus; }
        return tap (p, 0.f) + tap (p2, extra);
    }

private:
    void updateDeltas()
    {
        if (period < 2.f) { deltaPlus = deltaMinus = 0.f; return; }
        const float h = 0.5f * W, k = h / period;
        deltaPlus  = std::ceil (k) * period - h;     // tap2 por detrás de tap1 en k periodos
        deltaMinus = h - std::floor (k) * period;    // tap2 por delante de tap1 en k periodos
    }

    float tap (float ph, float extraDelay) const
    {
        const float d = ph * W + 1.f + extraDelay;
        const float rp = (float) wpos - d;
        const float fl = std::floor (rp);
        const int i0 = (int) fl;
        const float fr = rp - fl;
        const float a = buf[(size_t) (i0 & mask)];
        const float b = buf[(size_t) ((i0 + 1) & mask)];
        const float s = std::sin (3.14159265f * ph);
        return s * s * (a + (b - a) * fr);
    }

    std::vector<float> buf;
    int mask = 0, wpos = 0;
    float W = 1024.f, period = 0.f, deltaPlus = 0.f, deltaMinus = 0.f;
    float phase[NumHeads] {};
};
} // namespace hf
