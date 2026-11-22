cmake_minimum_required(VERSION 3.24)

project(QuadroPickupSwap VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

FetchContent_Declare(
    juce
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.4
    GIT_SHALLOW ON)
FetchContent_MakeAvailable(juce)

juce_add_plugin(QuadroPickupSwap
    COMPANY_NAME "Quadro"
    PRODUCT_NAME "Quadro Pickup Swap"
    BUNDLE_ID "com.quadro.pickupswap"
    PLUGIN_MANUFACTURER_CODE Qdro
    PLUGIN_CODE Qpsw
    FORMATS AU VST3
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    AU_MAIN_TYPE kAudioUnitType_Effect
    VST3_CATEGORIES Fx EQ
    COPY_PLUGIN_AFTER_BUILD TRUE)

target_sources(QuadroPickupSwap PRIVATE
    Source/PluginProcessor.cpp)

target_compile_definitions(QuadroPickupSwap PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(QuadroPickupSwap
    PRIVATE
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags)
