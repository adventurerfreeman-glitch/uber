cmake_minimum_required(VERSION 3.22)

project(UberknallInspired VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.6
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(UberknallInspired
    COMPANY_NAME "Independent Audio Lab"
    BUNDLE_ID com.independentaudiolab.uberknallinspired
    PLUGIN_MANUFACTURER_CODE Idal
    PLUGIN_CODE Ubkn
    FORMATS VST3
    PRODUCT_NAME "Uberknall Inspired"
    DESCRIPTION "An experimental Uberknall-inspired drive and amp tone stack"
    VERSION ${PROJECT_VERSION}
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
)

juce_generate_juce_header(UberknallInspired)

target_sources(UberknallInspired
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginProcessor.h
)

target_compile_definitions(UberknallInspired
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(UberknallInspired
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
