#include "HarmonyEngine.h"
#include <cstdio>
#include <vector>
#include <functional>

static int failures = 0;
constexpr double kPi = 3.14159265358979323846;

static void check (bool ok, const char* name, double got, double expected)
{
    std::printf ("[%s] %-46s got %9.3f  expected %9.3f\n", ok ? "PASS" : "FAIL", name, got, expected);
    if (! ok) ++failures;
}

// Frecuencia mediana medida en el tramo final de una señal.
static float measure (const std::vector<float>& x, double fs)
{
    hf::YinDetector y;
    y.prepare (fs, 65.f, 1000.f);
    const int n = y.frameSize();
    std::vector<float> f;
    for (int k = 0; k < 12; ++k)
    {
        const int end = (int) x.size() - k * 300;
        if (end - n < 0) break;
        float fr = 0, c = 0;
        if (y.detect (x.data() + end - n, fr, c)) f.push_back (fr);
    }
    if (f.empty()) return 0.f;
    std::sort (f.begin(), f.end());
    return f[f.size() / 2];
}

static std::vector<float> sine (double f, double fs, int n, float a = 0.5f)
{
    std::vector<float> x ((size_t) n);
    for (int i = 0; i < n; ++i) x[(size_t) i] = a * (float) std::sin (2.0 * kPi * f * i / fs);
    return x;
}

static std::vector<float> saw (double f, double fs, int n)
{
    std::vector<float> x ((size_t) n);
    double ph = 0;
    for (int i = 0; i < n; ++i) { x[(size_t) i] = 0.4f * (float) (2.0 * ph - 1.0); ph += f / fs; ph -= std::floor (ph); }
    return x;
}

struct Result { std::vector<float> ch[hf::kNumVoices + 1]; bool finite = true; };

static Result run (const std::vector<float>& in, double fs, const hf::EngineParams& p,
                   const std::function<void (hf::HarmonyEngine&)>& setup = {})
{
    hf::HarmonyEngine e;
    e.prepare (fs);
    e.setParams (p);
    if (setup) setup (e);
    Result r;
    for (auto& c : r.ch) c.resize (in.size());
    float o[hf::kNumVoices + 1];
    for (size_t i = 0; i < in.size(); ++i)
    {
        e.process (in[i], o);
        for (int v = 0; v <= hf::kNumVoices; ++v)
        {
            r.ch[v][i] = o[v];
            if (! std::isfinite (o[v])) r.finite = false;
        }
    }
    return r;
}

int main()
{
    const double fs = 48000.0;
    const int N = (int) (fs * 2.0);

    // 1) YIN
    for (double f : { 110.0, 220.0, 440.0, 880.0 })
    {
        const float m = measure (sine (f, fs, 20000), fs);
        char nm[64]; std::snprintf (nm, sizeof nm, "YIN sine %.0f Hz", f);
        check (std::fabs (m - f) / f < 0.005, nm, m, f);
    }

    // 2) Cuantizador y grados
    const uint16_t cmaj = hf::buildMask (0, hf::Major, 0);
    check (hf::quantize (61.3f, cmaj) == 62.f, "quantize 61.3 -> D (C major)", hf::quantize (61.3f, cmaj), 62);
    check (hf::shiftDegrees (60, 2, cmaj) == 64, "third above C in C major = E", hf::shiftDegrees (60, 2, cmaj), 64);
    check (hf::shiftDegrees (69, 2, cmaj) == 72, "third above A in C major = C", hf::shiftDegrees (69, 2, cmaj), 72);
    const uint16_t amin = hf::buildMask (9, hf::Minor, 0);
    check (amin == cmaj, "A minor mask == C major mask", amin, cmaj);

    // 3) Auto-tune: 225 Hz (A3 desafinado +39c) en C mayor -> 220 Hz; voz +2 grados -> C4
    {
        hf::EngineParams p;
        p.key = 0; p.scaleType = hf::Major; p.strength = 1.f; p.speedMs = 5.f; p.humanize = 0.f;
        p.voices[0].enabled = true; p.voices[0].degrees = 2;
        auto r = run (sine (225.0, fs, N), fs, p);
        check (r.finite, "no NaN/Inf", r.finite, 1);
        const float a = measure (r.ch[0], fs), b = measure (r.ch[1], fs);
        check (std::fabs (a - 220.0) / 220.0 < 0.005, "autotune 225 Hz -> 220 Hz (A3)", a, 220.0);
        check (std::fabs (b - 261.63) / 261.63 < 0.005, "voz +2 grados sobre A3 -> C4", b, 261.63);
    }

    // 4) Diente de sierra 150 Hz -> D3 (146.83 Hz) en modo robot
    {
        hf::EngineParams p;
        p.key = 0; p.scaleType = hf::Major; p.robot = true;
        auto r = run (saw (150.0, fs, N), fs, p);
        const float a = measure (r.ch[0], fs);
        check (std::fabs (a - 146.83) / 146.83 < 0.005, "robot saw 150 Hz -> D3", a, 146.83);
    }

    // 5) Modo MIDI: voz 1 sigue la nota E4 (329.63 Hz) con entrada de 220 Hz
    {
        hf::EngineParams p;
        p.midiMode = true; p.humanize = 0.f; p.speedMs = 5.f;
        p.voices[0].enabled = true;
        auto r = run (sine (220.0, fs, N), fs, p, [] (hf::HarmonyEngine& e) { e.noteOn (64); });
        const float b = measure (r.ch[1], fs);
        check (std::fabs (b - 329.63) / 329.63 < 0.005, "MIDI note E4 on voice 1", b, 329.63);
    }

    // 6) Silencio: salida finita y nula
    {
        hf::EngineParams p;
        p.voices[0].enabled = true; p.voices[0].degrees = 2;
        std::vector<float> z ((size_t) N, 0.f);
        auto r = run (z, fs, p);
        float mx = 0; for (auto s : r.ch[0]) mx = std::max (mx, std::fabs (s));
        check (r.finite && mx == 0.f, "silencio -> salida cero", mx, 0);
    }

    std::printf ("\n%s (%d fallos)\n", failures == 0 ? "TODOS LOS TESTS OK" : "HAY FALLOS", failures);
    return failures == 0 ? 0 : 1;
}
