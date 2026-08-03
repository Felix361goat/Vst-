cmake_minimum_required(VERSION 3.22)

# ---------------------------------------------------------------------------
# NOG Suite - a Serum-style wavetable synthesiser plugin.
#
# Build:
#   cmake -B build -DCMAKE_BUILD_TYPE=Release
#   cmake --build build --config Release
#
# The VST3 lands in:
#   build/NogSuite_artefacts/<Config>/VST3/NOG Suite.vst3
# ---------------------------------------------------------------------------

project(NogSuite VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Universal binaries on macOS so one build covers Intel and Apple Silicon.
if(APPLE AND NOT CMAKE_OSX_ARCHITECTURES)
    set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64" CACHE STRING "" FORCE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "" FORCE)
endif()

# Static-link the MSVC runtime so the plugin loads on machines that do not have
# the Visual C++ redistributable installed - most FL Studio users will not.
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

option(NOG_BUILD_TESTS "Build the NOG Suite unit tests" ON)
option(NOG_COPY_AFTER_BUILD "Install the plugin into the system plugin folder after building" OFF)

# ---------------------------------------------------------------------------
# JUCE
# ---------------------------------------------------------------------------
# Pinned deliberately: 8.0.15 is the last release of the well-trodden 8.x line.
# Moving to 9.x later is a one-line change to NOG_JUCE_TAG.
set(NOG_JUCE_TAG "8.0.15" CACHE STRING "JUCE git tag to build against")

include(FetchContent)

# A pre-cloned JUCE can be reused by setting FETCHCONTENT_SOURCE_DIR_JUCE,
# which keeps CI and container builds from re-downloading the framework.
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG ${NOG_JUCE_TAG}
    GIT_SHALLOW TRUE
    GIT_PROGRESS TRUE)

FetchContent_MakeAvailable(JUCE)

# ---------------------------------------------------------------------------
# Source lists
# ---------------------------------------------------------------------------
# Sources.cmake defines NOG_ENGINE_SOURCES (pure DSP/state, unit-testable) and
# NOG_PLUGIN_SOURCES (everything that needs the JUCE plugin-client defines).
include(${CMAKE_CURRENT_SOURCE_DIR}/Source/Sources.cmake)

# AU only exists on macOS; listing it elsewhere fails at configure time.
set(NOG_FORMATS VST3 Standalone)
if(APPLE)
    list(APPEND NOG_FORMATS AU)
endif()

# ---------------------------------------------------------------------------
# Embedded resources
# ---------------------------------------------------------------------------
# Compiled into the binary so the plugin has no external file dependencies -
# a plugin that needs files next to it breaks the moment a user moves it.
juce_add_binary_data(NogSuiteResources
    SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/Resources/background.jpg)

# Binary data is linked into a shared library, so it needs position-independent
# code on Linux.
set_target_properties(NogSuiteResources PROPERTIES POSITION_INDEPENDENT_CODE TRUE)

# ---------------------------------------------------------------------------
# The plugin target
# ---------------------------------------------------------------------------
juce_add_plugin(NogSuite
    PRODUCT_NAME                "NOG Suite"
    COMPANY_NAME                "FO Studio"
    BUNDLE_ID                   "com.fostudio.nogsuite"
    PLUGIN_MANUFACTURER_CODE    Fost      # unique per vendor
    PLUGIN_CODE                 Ngst      # unique per plugin
    FORMATS                     ${NOG_FORMATS}
    IS_SYNTH                    TRUE
    NEEDS_MIDI_INPUT            TRUE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD     ${NOG_COPY_AFTER_BUILD}
    VST3_CATEGORIES             "Instrument" "Synth"
    AU_MAIN_TYPE                kAudioUnitType_MusicDevice)

target_sources(NogSuite PRIVATE ${NOG_ENGINE_SOURCES} ${NOG_PLUGIN_SOURCES})
target_include_directories(NogSuite PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Source)

target_compile_definitions(NogSuite
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
        JUCE_REPORT_APP_USAGE=0
        JUCE_STRICT_REFCOUNTEDPOINTER=1)

target_link_libraries(NogSuite
    PRIVATE
        NogSuiteResources
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

if(NOG_BUILD_TESTS)
    enable_testing()
    add_subdirectory(Tests)
endif()
