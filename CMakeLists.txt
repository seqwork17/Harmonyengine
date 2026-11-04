cmake_minimum_required(VERSION 3.22)

# Runtime de MSVC enlazado estáticamente: el usuario NO necesita instalar el Visual C++ Redistributable.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>" CACHE STRING "" FORCE)

project(HarmonyForge VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(HF_BUILD_TESTS  "Compilar los tests del nucleo DSP" ON)
option(HF_BUILD_PLUGIN "Compilar el plugin (descarga JUCE)" ON)

# ---- Nucleo DSP (sin dependencias de JUCE) ---------------------------------
add_library(hf_dsp INTERFACE)
target_include_directories(hf_dsp INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/Source/DSP)

if(HF_BUILD_TESTS)
    enable_testing()
    add_executable(hf_dsp_test Tests/dsp_test.cpp)
    target_link_libraries(hf_dsp_test PRIVATE hf_dsp)
    add_test(NAME dsp_core COMMAND hf_dsp_test)
endif()

# ---- Plugin -----------------------------------------------------------------
if(HF_BUILD_PLUGIN)
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG        8.0.6
        GIT_SHALLOW    TRUE)
    FetchContent_MakeAvailable(JUCE)

    juce_add_plugin(HarmonyForge
        COMPANY_NAME               "HarmonyForge Audio"
        PRODUCT_NAME               "HarmonyForge"
        PLUGIN_MANUFACTURER_CODE   Hfrg
        PLUGIN_CODE                Hfg1
        FORMATS                    VST3 Standalone
        VST3_CATEGORIES            Fx "Pitch Shift"
        IS_SYNTH                   FALSE
        NEEDS_MIDI_INPUT           TRUE
        NEEDS_MIDI_OUTPUT          FALSE
        IS_MIDI_EFFECT             FALSE
        COPY_PLUGIN_AFTER_BUILD    FALSE)

    target_sources(HarmonyForge PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp)

    target_include_directories(HarmonyForge PRIVATE Source Source/DSP Source/Fx)

    target_compile_definitions(HarmonyForge PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0)

    target_link_libraries(HarmonyForge
        PRIVATE
            juce::juce_audio_utils
            juce::juce_dsp
            juce::juce_gui_extra
            hf_dsp
        PUBLIC
            juce::juce_recommended_config_flags
            juce::juce_recommended_lto_flags)
endif()
