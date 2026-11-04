#pragma once
#include <juce_core/juce_core.h>
#include <vector>
#include <utility>
#include "Parameters.h"

namespace hf
{
using PV = std::vector<std::pair<juce::String, float>>;

struct Preset
{
    const char* name;
    PV values;
};

// Añade los parámetros de una voz.
inline void addVoice (PV& p, int v, float volDb, int deg, int oct, int semi, float cents, float pan)
{
    p.push_back ({ pid::voice (v, pid::vEnable), 1.f });
    p.push_back ({ pid::voice (v, pid::vVol), volDb });
    p.push_back ({ pid::voice (v, pid::vDeg), (float) deg });
    p.push_back ({ pid::voice (v, pid::vOct), (float) oct });
    p.push_back ({ pid::voice (v, pid::vSemi), (float) semi });
    p.push_back ({ pid::voice (v, pid::vCents), cents });
    p.push_back ({ pid::voice (v, pid::vPan), pan });
}

inline const std::vector<Preset>& presets()
{
    static const std::vector<Preset> list = []
    {
        std::vector<Preset> L;

        L.push_back ({ "Init - Clean Tune", {} });

        L.push_back ({ "Natural Vocal Fix", {
            { pid::strength, 80.f }, { pid::speed, 70.f }, { pid::humanize, 45.f }, { pid::revMix, 10.f } } });

        L.push_back ({ "Hard Tune Robot", {
            { pid::robot, 1.f }, { pid::strength, 100.f }, { pid::speed, 0.f }, { pid::humanize, 0.f },
            { pid::satMix, 25.f }, { pid::chMix, 20.f } } });

        {
            PV p { { pid::strength, 90.f }, { pid::speed, 20.f }, { pid::revMix, 14.f } };
            addVoice (p, 0, -5.f, 2, 0, 0, -3.f, -0.6f);
            addVoice (p, 1, -5.f, -2, 0, 0, 3.f, 0.6f);
            L.push_back ({ "Pop Thirds", p });
        }
        {
            PV p { { pid::strength, 85.f }, { pid::speed, 35.f }, { pid::humanize, 60.f }, { pid::revMix, 22.f }, { pid::revSize, 70.f }, { pid::chMix, 18.f } };
            const int deg[8] = { 2, 4, -2, -4, 0, 0, 2, -3 };
            const int oct[8] = { 0, 0, 0, 0, 1, -1, 1, -1 };
            const float pan[8] = { -0.6f, 0.6f, -0.9f, 0.9f, -0.3f, 0.3f, -0.15f, 0.15f };
            for (int v = 0; v < 8; ++v)
                addVoice (p, v, -8.f, deg[v], oct[v], 0, (v % 2 ? 7.f : -7.f), pan[v]);
            L.push_back ({ "Choir Stack (8 voices)", p });
        }
        {
            PV p { { pid::strength, 100.f }, { pid::speed, 12.f }, { pid::satMix, 30.f }, { pid::satDrive, 35.f } };
            addVoice (p, 0, -6.f, 4, 0, 0, 0.f, -0.5f);
            addVoice (p, 1, -6.f, -4, 0, 0, 0.f, 0.5f);
            L.push_back ({ "Power Fifths", p });
        }
        {
            PV p { { pid::strength, 95.f }, { pid::speed, 15.f } };
            addVoice (p, 0, -8.f, 0, 1, 0, 0.f, -0.4f);
            addVoice (p, 1, -8.f, 0, -1, 0, 0.f, 0.4f);
            L.push_back ({ "Octave Doubler", p });
        }
        {
            PV p { { pid::scale, (float) Minor }, { pid::strength, 100.f }, { pid::speed, 8.f }, { pid::humanize, 10.f },
                   { pid::dlyMix, 22.f }, { pid::dlyTime, 380.f }, { pid::revMix, 25.f }, { pid::revSize, 80.f }, { pid::eqLow, 2.f } };
            addVoice (p, 0, -6.f, 0, -1, 0, 0.f, 0.f);
            addVoice (p, 1, -9.f, 2, 0, 0, 5.f, -0.7f);
            addVoice (p, 2, -9.f, 2, 1, 0, -5.f, 0.7f);
            L.push_back ({ "Dark Trap Layer", p });
        }
        {
            PV p { { pid::mode, 1.f }, { pid::strength, 90.f }, { pid::speed, 25.f }, { pid::revMix, 15.f } };
            for (int v = 0; v < 4; ++v)
                addVoice (p, v, -6.f, 0, 0, 0, (v % 2 ? 4.f : -4.f), -0.6f + 0.4f * (float) v);
            L.push_back ({ "MIDI Harmonizer (4 voices)", p });
        }
        return L;
    }();
    return list;
}
} // namespace hf
