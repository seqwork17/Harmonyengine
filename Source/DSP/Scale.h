#pragma once
#include <cmath>
#include <cstdint>
#include <initializer_list>

namespace hf
{
enum ScaleType
{
    Chromatic = 0, Major, Minor, HarmonicMinor, MelodicMinor, Dorian, Phrygian,
    Lydian, Mixolydian, PentatonicMajor, PentatonicMinor, Blues, Custom, NumScaleTypes
};

inline constexpr const char* kScaleNames[NumScaleTypes] = {
    "Chromatic", "Major", "Minor", "Harmonic Minor", "Melodic Minor", "Dorian", "Phrygian",
    "Lydian", "Mixolydian", "Pentatonic Major", "Pentatonic Minor", "Blues", "Custom (notes)"
};

inline constexpr const char* kNoteNames[12] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };

inline uint16_t maskFrom (std::initializer_list<int> l)
{
    uint16_t m = 0;
    for (int i : l) m = (uint16_t) (m | (1u << i));
    return m;
}

// Máscara de 12 bits (bit = clase de altura absoluta, 0 = C) para tonalidad + escala.
inline uint16_t buildMask (int key, int type, uint16_t custom)
{
    if (type == Custom)
        return (uint16_t) (custom & 0x0FFF);

    uint16_t base = 0x0FFF;
    switch (type)
    {
        case Major:           base = maskFrom ({ 0, 2, 4, 5, 7, 9, 11 }); break;
        case Minor:           base = maskFrom ({ 0, 2, 3, 5, 7, 8, 10 }); break;
        case HarmonicMinor:   base = maskFrom ({ 0, 2, 3, 5, 7, 8, 11 }); break;
        case MelodicMinor:    base = maskFrom ({ 0, 2, 3, 5, 7, 9, 11 }); break;
        case Dorian:          base = maskFrom ({ 0, 2, 3, 5, 7, 9, 10 }); break;
        case Phrygian:        base = maskFrom ({ 0, 1, 3, 5, 7, 8, 10 }); break;
        case Lydian:          base = maskFrom ({ 0, 2, 4, 6, 7, 9, 11 }); break;
        case Mixolydian:      base = maskFrom ({ 0, 2, 4, 5, 7, 9, 10 }); break;
        case PentatonicMajor: base = maskFrom ({ 0, 2, 4, 7, 9 }); break;
        case PentatonicMinor: base = maskFrom ({ 0, 3, 5, 7, 10 }); break;
        case Blues:           base = maskFrom ({ 0, 3, 5, 6, 7, 10 }); break;
        default: break;
    }
    key = ((key % 12) + 12) % 12;
    const uint32_t m = base;
    return (uint16_t) (((m << key) | (m >> (12 - key))) & 0x0FFFu);
}

inline bool isAllowed (int note, uint16_t mask)
{
    return (mask >> (((note % 12) + 12) % 12)) & 1;
}

// Nota MIDI (float) -> nota permitida más cercana. mask==0 => sin cambio.
inline float quantize (float midi, uint16_t mask)
{
    if (mask == 0) return midi;
    const int n0 = (int) std::lround (midi);
    for (int d = 0; d <= 6; ++d)
    {
        const bool lo = isAllowed (n0 - d, mask), hi = isAllowed (n0 + d, mask);
        if (lo && hi) return std::fabs (midi - (float) (n0 - d)) <= std::fabs (midi - (float) (n0 + d)) ? (float) (n0 - d) : (float) (n0 + d);
        if (lo) return (float) (n0 - d);
        if (hi) return (float) (n0 + d);
    }
    return midi;
}

// Desplaza una nota "steps" grados de la escala (positivo/negativo).
inline int shiftDegrees (int note, int steps, uint16_t mask)
{
    if (mask == 0) return note + steps;
    const int dir = steps >= 0 ? 1 : -1;
    for (int s = 0; s < std::abs (steps); ++s)
    {
        int guard = 0;
        do { note += dir; } while (! isAllowed (note, mask) && ++guard < 12);
    }
    return note;
}
} // namespace hf
