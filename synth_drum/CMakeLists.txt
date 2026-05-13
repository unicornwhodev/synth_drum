cmake_minimum_required(VERSION 3.22)

project(UWdeVST_drum VERSION 1.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(MDS_ENABLE_VST2 "Enable legacy VST2 format (requires legacy JUCE + VST2 SDK)" OFF)

# ---------------------------------------------------------------------------
# JUCE: use an explicit checkout when provided, otherwise fall back to a
# repository-local copy or FetchContent.
# ---------------------------------------------------------------------------
set(UWDEVST_JUCE_DIR "" CACHE PATH "Optional path to a JUCE checkout to use instead of FetchContent")

if (UWDEVST_JUCE_DIR AND EXISTS "${UWDEVST_JUCE_DIR}/CMakeLists.txt")
    add_subdirectory("${UWDEVST_JUCE_DIR}" JUCE)
elseif (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/JUCE/CMakeLists.txt")
    add_subdirectory(JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(
        JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG        8.0.4
        GIT_SHALLOW    TRUE
    )
    FetchContent_MakeAvailable(JUCE)
endif()

set(MDS_PLUGIN_FORMATS Standalone VST3)
if (MDS_ENABLE_VST2)
    list(APPEND MDS_PLUGIN_FORMATS VST)
endif()

set(_uwdevst_default_assets_dir "${CMAKE_CURRENT_SOURCE_DIR}/../assets versions png")
if (NOT EXISTS "${_uwdevst_default_assets_dir}" AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../assets_versions_png")
    set(_uwdevst_default_assets_dir "${CMAKE_CURRENT_SOURCE_DIR}/../assets_versions_png")
endif()

set(UWDEVST_SHARED_ASSETS_DIR "${_uwdevst_default_assets_dir}" CACHE PATH "Path to the shared UI/assets directory")
if (IS_ABSOLUTE "${UWDEVST_SHARED_ASSETS_DIR}")
    set(ASSETS_DIR "${UWDEVST_SHARED_ASSETS_DIR}")
else()
    get_filename_component(ASSETS_DIR "${UWDEVST_SHARED_ASSETS_DIR}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
endif()

juce_add_plugin(UWdeVST_drum
    COMPANY_NAME "Musique"
    BUNDLE_ID "com.uwdevst.drum"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE MQDS
    PLUGIN_CODE MDS1
    FORMATS ${MDS_PLUGIN_FORMATS}
    PRODUCT_NAME "UWdeVST_drum"
    ICON_BIG   "${ASSETS_DIR}/logo,favic/logo_drum.png"
    ICON_SMALL "${ASSETS_DIR}/logo,favic/logo_drum.png"
)

juce_generate_juce_header(UWdeVST_drum)

target_sources(UWdeVST_drum PRIVATE
    Source/PluginEditor.cpp
    Source/PluginEditor.h
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/Engine/DrumDefs.cpp
    Source/Engine/DrumDefs.h
    Source/Engine/DrumConstants.h
    Source/Engine/DrumSynthVoice.cpp
    Source/Engine/DrumSynthVoice.h
    Source/Engine/FactoryPresets.cpp
    Source/Engine/FactoryPresets.h
    Source/Engine/FxProcessors.h
    Source/Engine/SinTable.h

    # V5 UI components (live in ../new composants relative to this CMakeLists.txt)
    "${CMAKE_CURRENT_SOURCE_DIR}/../new composants/KnobComponentV5.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../new composants/KnobComponentV5.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../new composants/FaderComponentV5.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../new composants/FaderComponentV5.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../new composants/VUMeterComponentV5.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../new composants/VUMeterComponentV5.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../new composants/EnvelopeDisplayComponentV5.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../new composants/EnvelopeDisplayComponentV5.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../new composants/OutputMeterComponentV5.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../new composants/OutputMeterComponentV5.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../new composants/ToggleButtonComponentV5.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../new composants/ToggleButtonComponentV5.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../new composants/SelectorComponentV5.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../new composants/SelectorComponentV5.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../new composants/UIThemeV5.h"
)

# Include path so `#include "new composants/X.h"` (used by PluginEditor.h)
# and `#include "X.h"` (used internally between V5 headers) both resolve.
target_include_directories(UWdeVST_drum PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/.."
    "${CMAKE_CURRENT_SOURCE_DIR}/../new composants"
)

target_compile_definitions(UWdeVST_drum PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_REPORT_APP_USAGE=0
)

# ---------------------------------------------------------------------------
# MSVC: Release optimizations
# ---------------------------------------------------------------------------
if (MSVC)
    target_compile_options(UWdeVST_drum PRIVATE
        /utf-8
        $<$<CONFIG:Release>:/O2 /fp:fast /GL>
    )
    target_link_options(UWdeVST_drum PRIVATE
        $<$<CONFIG:Release>:/LTCG>
    )
endif ()

# ---------------------------------------------------------------------------
# Binary assets (icons)
# ---------------------------------------------------------------------------
juce_add_binary_data(DrumAssets SOURCES
    # Commune icons
    "${ASSETS_DIR}/icons/00_communes/001_level_00001_.png"
    "${ASSETS_DIR}/icons/00_communes/002_tune_00001_.png"
    "${ASSETS_DIR}/icons/00_communes/003_attack_00001_.png"
    "${ASSETS_DIR}/icons/00_communes/004_decay_00001_.png"
    "${ASSETS_DIR}/icons/00_communes/005_sustain_00001_.png"
    "${ASSETS_DIR}/icons/00_communes/006_release_00001_.png"
    "${ASSETS_DIR}/icons/00_communes/007_cutoff_00001_.png"
    "${ASSETS_DIR}/icons/00_communes/008_pan_00001_.png"
    "${ASSETS_DIR}/icons/00_communes/009_brightness_00001_.png"
    "${ASSETS_DIR}/icons/00_communes/010_character_00001_.png"
    "${ASSETS_DIR}/icons/00_communes/006_gain_volume.png"
    "${ASSETS_DIR}/icons/00_communes/012_output_routing.png"
    "${ASSETS_DIR}/icons/00_communes/013_keyboard_toggle.png"
    # Drum category icons
    "${ASSETS_DIR}/icons/01_mds/014_categorie_kick.png"
    "${ASSETS_DIR}/icons/01_mds/015_categorie_snare.png"
    "${ASSETS_DIR}/icons/01_mds/016_categorie_hi_hat.png"
    "${ASSETS_DIR}/icons/01_mds/016_categorie_hi_hat.png"
    "${ASSETS_DIR}/icons/01_mds/018_categorie_tom.png"
    "${ASSETS_DIR}/icons/01_mds/019_categorie_fx.png"
    "${ASSETS_DIR}/icons/01_mds/020_single_note.png"
    # Drum FX icons
    "${ASSETS_DIR}/icons/01_mds/021_compressor.png"
    "${ASSETS_DIR}/icons/01_mds/022_saturator.png"
    "${ASSETS_DIR}/icons/09_fx/044_envelope_amount_00001_.png"
    # Drum macro icons
    "${ASSETS_DIR}/icons/01_mds/024_macro_punch.png"
    "${ASSETS_DIR}/icons/01_mds/025_macro_weight.png"
    "${ASSETS_DIR}/icons/01_mds/026_macro_air.png"
    "${ASSETS_DIR}/icons/09_fx/037_drive_saturation_00001_.png"
    # Drum control/fx dial icons
    "${ASSETS_DIR}/icons/01_mds/012_pitch_drop_00001_.png"
    "${ASSETS_DIR}/icons/01_mds/013_click_00001_.png"
    "${ASSETS_DIR}/icons/01_mds/014_noise_00001_.png"
    "${ASSETS_DIR}/icons/09_fx/024_threshold_00001_.png"
    "${ASSETS_DIR}/icons/09_fx/025_ratio_00001_.png"
    "${ASSETS_DIR}/icons/09_fx/026_makeup_gain_00001_.png"
    "${ASSETS_DIR}/icons/09_fx/037_drive_saturation_00001_.png"
    "${ASSETS_DIR}/icons/09_fx/038_blend_mix_00001_.png"
)

target_link_libraries(UWdeVST_drum PRIVATE
    DrumAssets
    juce::juce_audio_utils
    juce::juce_dsp
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags
    juce::juce_recommended_lto_flags
)

juce_add_console_app(UWdeVST_drum_renderer
    PRODUCT_NAME "UWdeVST_drum_renderer"
)

juce_generate_juce_header(UWdeVST_drum_renderer)

target_sources(UWdeVST_drum_renderer PRIVATE
    Source/Render/DrumManifestRenderer.cpp
    Source/Engine/DrumDefs.cpp
    Source/Engine/DrumDefs.h
    Source/Engine/DrumConstants.h
    Source/Engine/DrumSynthVoice.cpp
    Source/Engine/DrumSynthVoice.h
    Source/Engine/FactoryPresets.cpp
    Source/Engine/FactoryPresets.h
    Source/Engine/FxProcessors.h
    Source/Engine/SinTable.h
)

target_compile_definitions(UWdeVST_drum_renderer PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
)

target_link_libraries(UWdeVST_drum_renderer PRIVATE
    juce::juce_audio_formats
    juce::juce_dsp
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags
    juce::juce_recommended_lto_flags
)

juce_add_console_app(UWdeVST_drum_tests
    PRODUCT_NAME "UWdeVST_drum_tests"
)

juce_generate_juce_header(UWdeVST_drum_tests)

target_sources(UWdeVST_drum_tests PRIVATE
    Source/Tests/DrumProductionTests.cpp
    Source/PluginEditor.cpp
    Source/PluginEditor.h
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/Engine/DrumDefs.cpp
    Source/Engine/DrumDefs.h
    Source/Engine/DrumConstants.h
    Source/Engine/DrumSynthVoice.cpp
    Source/Engine/DrumSynthVoice.h
    Source/Engine/FactoryPresets.cpp
    Source/Engine/FactoryPresets.h
    Source/Engine/FxProcessors.h
    Source/Engine/SinTable.h

    # V5 UI components (same set as the VST3 target)
    "${CMAKE_CURRENT_SOURCE_DIR}/../new composants/KnobComponentV5.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../new composants/FaderComponentV5.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../new composants/VUMeterComponentV5.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../new composants/EnvelopeDisplayComponentV5.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../new composants/OutputMeterComponentV5.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../new composants/ToggleButtonComponentV5.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../new composants/SelectorComponentV5.cpp"
)

target_include_directories(UWdeVST_drum_tests PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/.."
    "${CMAKE_CURRENT_SOURCE_DIR}/../new composants"
)

target_compile_definitions(UWdeVST_drum_tests PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
)

target_link_libraries(UWdeVST_drum_tests PRIVATE
    DrumAssets
    juce::juce_audio_utils
    juce::juce_dsp
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags
    juce::juce_recommended_lto_flags
)

if (MSVC)
    target_compile_options(UWdeVST_drum_renderer PRIVATE /utf-8)
    target_compile_options(UWdeVST_drum_tests PRIVATE /utf-8)
endif ()
