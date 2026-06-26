cmake_minimum_required(VERSION 3.22)

project(SpunOnWax VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---------------------------------------------------------------------------
# JUCE
#
# By default we pull JUCE via FetchContent so the project builds with no
# manual setup. If you already have a local JUCE checkout, point the cache
# variable SPUNONWAX_JUCE_PATH at it to skip the download, e.g.:
#   cmake -B build -DSPUNONWAX_JUCE_PATH=/path/to/JUCE
# ---------------------------------------------------------------------------
set(SPUNONWAX_JUCE_PATH "" CACHE PATH "Path to an existing JUCE checkout (optional)")

if(SPUNONWAX_JUCE_PATH)
    message(STATUS "SpunOnWax: using local JUCE at ${SPUNONWAX_JUCE_PATH}")
    add_subdirectory(${SPUNONWAX_JUCE_PATH} ${CMAKE_BINARY_DIR}/JUCE)
else()
    include(FetchContent)
    message(STATUS "SpunOnWax: fetching JUCE via FetchContent")
    FetchContent_Declare(
        JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG        8.0.4
        GIT_SHALLOW    TRUE
    )
    FetchContent_MakeAvailable(JUCE)
endif()

# ---------------------------------------------------------------------------
# Plugin target
# ---------------------------------------------------------------------------
juce_add_plugin(SpunOnWax
    COMPANY_NAME              "SpunOnWax"
    BUNDLE_ID                 com.spunonwax.spunonwax
    PLUGIN_MANUFACTURER_CODE  Sown
    PLUGIN_CODE               Spwx
    FORMATS                   VST3 AU Standalone
    PRODUCT_NAME              "Spun on Wax"
    IS_SYNTH                  FALSE
    NEEDS_MIDI_INPUT          FALSE
    NEEDS_MIDI_OUTPUT         FALSE
    IS_MIDI_EFFECT            FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD   FALSE
    VST3_CATEGORIES           "Fx" "Distortion"
    AU_MAIN_TYPE              "kAudioUnitType_Effect")

juce_generate_juce_header(SpunOnWax)

target_sources(SpunOnWax PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_compile_definitions(SpunOnWax PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(SpunOnWax
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
