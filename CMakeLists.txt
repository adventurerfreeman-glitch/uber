cmake_minimum_required(VERSION 3.22)
project(UberknallPreamp VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set JUCE_DIR to your JUCE checkout, or add JUCE as a submodule.
if(NOT DEFINED JUCE_DIR)
    message(FATAL_ERROR "Set JUCE_DIR to your JUCE source directory.")
endif()
add_subdirectory(${JUCE_DIR} JUCE)

juce_add_plugin(UberknallPreamp
    COMPANY_NAME "Prototype Audio"
    PLUGIN_MANUFACTURER_CODE PtAu
    PLUGIN_CODE UbKn
    FORMATS VST3 Standalone
    PRODUCT_NAME "Uberknall Preamp"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
)

juce_generate_juce_header(UberknallPreamp)

target_sources(UberknallPreamp PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
)

target_compile_definitions(UberknallPreamp PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(UberknallPreamp PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)
