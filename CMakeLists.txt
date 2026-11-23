cmake_minimum_required(VERSION 3.24)

project(QuadroPickupSwap VERSION 0.2.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(QUADRO_UI_SNAPSHOT "Arma tambien la herramienta que guarda una imagen de la interfaz" OFF)

include(FetchContent)

FetchContent_Declare(
    juce
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.4
    GIT_SHALLOW ON)
FetchContent_MakeAvailable(juce)

if(APPLE)
    set(QUADRO_FORMATS AU VST3)
else()
    set(QUADRO_FORMATS VST3)
endif()

juce_add_plugin(QuadroPickupSwap
    COMPANY_NAME "Quadro"
    PRODUCT_NAME "Quadro Pickup Swap"
    BUNDLE_ID "com.quadro.pickupswap"
    PLUGIN_MANUFACTURER_CODE Qdro
    PLUGIN_CODE Qpsw
    FORMATS ${QUADRO_FORMATS}
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    AU_MAIN_TYPE kAudioUnitType_Effect
    VST3_CATEGORIES Fx EQ
    COPY_PLUGIN_AFTER_BUILD TRUE)

# Fotos, tipografias e isotipo de Quadro, empaquetados dentro del plugin.
juce_add_binary_data(QuadroAssets SOURCES
    Resources/brand/quadro-isotipo.svg
    Resources/fonts/ArchivoExpanded-ExtraBold.ttf
    Resources/fonts/StackSansText-Regular.ttf
    Resources/fonts/StackSansText-Medium.ttf
    Resources/fonts/IBMPlexMono-Regular.ttf
    Resources/fonts/IBMPlexMono-Medium.ttf
    Resources/images/pickup-strat.jpg
    Resources/images/pickup-tele.jpg
    Resources/images/pickup-p90.jpg
    Resources/images/pickup-humbucker.jpg
    Resources/images/pickup-split.jpg)

target_sources(QuadroPickupSwap PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_compile_definitions(QuadroPickupSwap PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(QuadroPickupSwap
    PRIVATE
        QuadroAssets
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags)

if(QUADRO_UI_SNAPSHOT)
    # Programa de consola que crea la interfaz fuera de un DAW y guarda una imagen PNG (para revisarla).
    juce_add_console_app(QuadroUISnapshot PRODUCT_NAME "Quadro UI Snapshot")
    target_sources(QuadroUISnapshot PRIVATE
        tools/ui_snapshot.cpp
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp)
    target_compile_definitions(QuadroUISnapshot PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JucePlugin_Name="Quadro Pickup Swap"
        JucePlugin_VersionString="${PROJECT_VERSION}")
    target_link_libraries(QuadroUISnapshot PRIVATE
        QuadroAssets
        juce::juce_audio_utils
        juce::juce_recommended_config_flags)
endif()
