#pragma once
#include <vector>
#include <cmath>
#include <algorithm>

namespace hf
{
// Detector de pitch YIN (de Cheveigné & Kawahara, 2002) con interpolación parabólica.
class YinDetector
{
public:
    void prepare (double sampleRate, float fMin = 65.f, float fMax = 1000.f)
    {
        fs = sampleRate;
        tauMax = std::max (8, (int) (fs / fMin));
        tauMin = std::max (2, (int) (fs / fMax));
        W = tauMax;
        cmnd.assign ((size_t) tauMax + 2, 1.f);
    }

    // Nº de muestras que necesita detect().
    int frameSize() const { return W + tauMax + 2; }

    // x: frameSize() muestras (las más recientes al final).
    bool detect (const float* x, float& freq, float& confidence, float threshold = 0.15f)
    {
        float running = 0.f;
        cmnd[0] = 1.f;
        for (int tau = 1; tau < tauMax; ++tau)
        {
            float d = 0.f;
            for (int j = 0; j < W; ++j)
            {
                const float e = x[j] - x[j + tau];
                d += e * e;
            }
            running += d;
            cmnd[(size_t) tau] = running > 1e-12f ? d * (float) tau / running : 1.f;
        }

        int best = -1;
        for (int tau = tauMin; tau < tauMax - 1; ++tau)
        {
            if (cmnd[(size_t) tau] < threshold)
            {
                while (tau + 1 < tauMax - 1 && cmnd[(size_t) tau + 1] < cmnd[(size_t) tau]) ++tau;
                best = tau;
                break;
            }
        }
        if (best < 0)
        {
            // Sin cruce de umbral: acepta el mínimo global solo si es razonablemente periódico.
            float mn = 1e9f; int at = -1;
            for (int tau = tauMin; tau < tauMax - 1; ++tau)
                if (cmnd[(size_t) tau] < mn) { mn = cmnd[(size_t) tau]; at = tau; }
            if (at < 0 || mn > 0.3f) return false;
            best = at;
        }

        float tauF = (float) best;
        if (best > 1 && best < tauMax - 1)
        {
            const float a = cmnd[(size_t) best - 1], b = cmnd[(size_t) best], c = cmnd[(size_t) best + 1];
            const float den = a - 2.f * b + c;
            if (std::fabs (den) > 1e-9f) tauF += 0.5f * (a - c) / den;
        }
        if (tauF <= 0.f) return false;
        freq = (float) fs / tauF;
        confidence = 1.f - cmnd[(size_t) best];
        return true;
    }

private:
    double fs = 16000.0;
    int tauMax = 246, tauMin = 16, W = 246;
    std::vector<float> cmnd;
};
} // namespace hf
