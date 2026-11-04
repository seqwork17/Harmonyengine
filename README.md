# HarmonyForge — Auto-Tune + Harmonizer de 8 voces (VST3)

Plugin VST3 para Windows 10/11 (FL Studio), hecho con C++17, JUCE 8 y CMake.

## Estado actual (Etapa 1)

| Parte | Estado |
|---|---|
| Núcleo DSP (detección YIN, cuantizador de escalas, pitch shifter sincronizado al periodo, 8 voces, modo MIDI) | **Compilado y probado** con `Tests/dsp_test.cpp` (14 tests OK, error de afinación < 0,5 %) |
| Capa JUCE (parámetros, efectos, procesador, presets, interfaz) | **Escrita, aún NO compilada** — se compila en Windows con la acción de GitHub incluida |
| Probado dentro de FL Studio | **No** |

No se debe considerar terminado hasta que el workflow compile sin errores y se pruebe en FL Studio.

## Obtener el .vst3 sin instalar nada (recomendado)

1. Crea un repositorio en GitHub y sube **todo el contenido** de esta carpeta (incluida `.github`).
2. Ve a la pestaña **Actions** → ejecución "Build Windows VST3" (se lanza sola con cada push).
3. Cuando termine, descarga el artefacto **HarmonyForge-VST3-Windows** (zip con `HarmonyForge.vst3`).
4. Si la compilación falla, copia el error y envíamelo: lo corrijo en la siguiente etapa.

## Compilar en tu PC

Requisitos: Visual Studio 2022 (carga de trabajo "Desarrollo para el escritorio con C++"), CMake ≥ 3.22 y Git.

```bat
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
```

Resultado: `build\HarmonyForge_artefacts\Release\VST3\HarmonyForge.vst3` (y una versión Standalone para probar sin DAW).
La primera configuración descarga JUCE 8.0.6 (necesita internet).

## Instalación en FL Studio

1. Copia la carpeta `HarmonyForge.vst3` a `C:\Program Files\Common Files\VST3\`.
2. FL Studio → **Options → Manage plugins → Find installed plugins** (con "Verify plugins" activado).
3. Añade **HarmonyForge** como efecto en una pista del mezclador, en el slot donde va tu voz.
4. Modo MIDI: en el engranaje del plugin (Wrapper settings) asigna un **puerto MIDI de entrada** y envía notas desde un canal del Channel Rack con el mismo puerto de salida.

## Uso rápido

- **Key / Scale**: define las notas permitidas. Con *Custom (notes)* eliges nota a nota con los 12 botones.
- **Correction / Retune / Humanize**: intensidad, velocidad (0 ms = efecto robótico) y naturalidad.
- **Robot**: fuerza corrección total e instantánea.
- **Harmony Mode**: *Auto* arma las voces por grados de la escala (Interval); *MIDI* las dirige con las notas que mantengas pulsadas.
- **Voice 1–8**: on/off, nivel, pan, intervalo en grados, octava, semitonos y ajuste fino (±100 ct).
- **Latency**: Low 14 ms / Normal 24 ms / Smooth 36 ms. El plugin informa su latencia al host (compensación automática) y alinea la señal seca.
- Presets de fábrica en el selector superior.

## Arquitectura

```
Source/DSP/   Scale.h  YinDetector.h  MultiShifter.h  HarmonyEngine.h   (C++ puro, probado)
Source/Fx/    StereoDelay.h
Source/       Parameters.h  Presets.h  PluginProcessor.*  PluginEditor.*
Tests/        dsp_test.cpp
```

## Pendiente (siguientes etapas)

- **Control de formantes real** (preservación de formante con LPC/cepstrum). Aún no está implementado, por eso no hay mando de formantes todavía.
- Sobremuestreo en la saturación, suavizado de coeficientes del EQ y mejora de calidad del shifter en voces graves/agudas.
- Verificación en FL Studio (latencia, automatización, MIDI), pruebas con voz real y ajuste fino de la interfaz.
- Instalador (.exe) y firma de código.

## Licencia

JUCE se distribuye bajo GPLv3 o licencia comercial. Para vender el plugin sin publicar su código necesitas una licencia comercial de JUCE.
