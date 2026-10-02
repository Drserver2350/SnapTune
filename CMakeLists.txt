cmake_minimum_required(VERSION 3.22)
project(SnapTune VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.4)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(SnapTune
    COMPANY_NAME "Atomicmail"
    PLUGIN_MANUFACTURER_CODE Atmc
    PLUGIN_CODE Sntn
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "SnapTune"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE)

target_sources(SnapTune PRIVATE
    Source/PluginProcessor.cpp)

target_compile_definitions(SnapTune PUBLIC
    JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(SnapTune PRIVATE
    juce::juce_audio_utils juce::juce_dsp
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags juce::juce_recommended_warning_flags)
