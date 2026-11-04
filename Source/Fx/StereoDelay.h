#pragma once
#include <vector>
#include <cmath>

namespace hf
{
class StereoDelay
{
public:
    void prepare (double sampleRate)
    {
        fs = sampleRate;
        size = (int) (sampleRate * 2.0) + 4;
        for (auto& b : buf) b.assign ((size_t) size, 0.f);
        w = 0; lpL = lpR = 0.f;
        lpA = 1.f - std::exp (-6.2831853f * 5000.f / (float) fs);
        smoothA = 1.f - std::exp (-1.f / (0.03f * (float) fs));
        cur = target = (float) (0.3 * fs);
    }

    void setParams (float ms, float feedback, float mixAmount)
    {
        target = ms * 0.001f * (float) fs;
        fb = feedback;
        mix = mixAmount;
    }

    bool active() const { return mix > 0.0005f; }

    void process (float* l, float* r, int n)
    {
        for (int i = 0; i < n; ++i)
        {
            cur += (target - cur) * smoothA;
            float rp = (float) w - cur;
            if (rp < 0.f) rp += (float) size;
            int i0 = (int) rp;
            const float fr = rp - (float) i0;
            if (i0 >= size) i0 -= size;
            const int i1 = i0 + 1 >= size ? 0 : i0 + 1;

            const float dl = buf[0][(size_t) i0] + (buf[0][(size_t) i1] - buf[0][(size_t) i0]) * fr;
            const float dr = buf[1][(size_t) i0] + (buf[1][(size_t) i1] - buf[1][(size_t) i0]) * fr;
            lpL += lpA * (dl - lpL);
            lpR += lpA * (dr - lpR);

            buf[0][(size_t) w] = l[i] + lpR * fb;   // ping-pong
            buf[1][(size_t) w] = r[i] + lpL * fb;
            if (++w >= size) w = 0;

            l[i] += dl * mix;
            r[i] += dr * mix;
        }
    }

private:
    double fs = 48000.0;
    int size = 96004, w = 0;
    std::vector<float> buf[2];
    float cur = 14400.f, target = 14400.f, fb = 0.f, mix = 0.f, lpL = 0.f, lpR = 0.f, lpA = 0.4f, smoothA = 0.001f;
};
} // namespace hf
