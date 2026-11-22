#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "PickupModels.h"

#include <array>
#include <atomic>

// Quadro Pickup Swap: hace que una capsula suene como otra.
//  - Desde:  la capsula que tiene la guitarra que estas tocando.
//  - Hacia:  la capsula que quieres escuchar.
//  - Mezcla: cuanto de la senal convertida se escucha.
//  - Brillo: ajuste fino de agudos (+/- 6 dB sobre 5 kHz).
//  - Salida: volumen final.
// Todo son filtros: no agrega retraso.
class PickupSwapProcessor : public juce::AudioProcessor
{
public:
    PickupSwapProcessor();
    ~PickupSwapProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    juce::AudioProcessorValueTreeState apvts;

private:
    static constexpr int kMaxChannels = 2;
    static constexpr double kBrightnessFrequency = 5000.0;

    void updateFilters (bool force);

    double sampleRate = 48000.0;

    std::array<pickup::Chain, kMaxChannels> current {};
    std::array<pickup::Chain, kMaxChannels> previous {};
    int fadeLength = 960;
    int fadeRemaining = 0;

    int activeFrom = -1;
    int activeTo = -1;
    float activeBrightness = 1000.0f;

    juce::SmoothedValue<float> mixSmoothed;
    juce::SmoothedValue<float> gainSmoothed;

    std::atomic<float>* fromParam = nullptr;
    std::atomic<float>* toParam = nullptr;
    std::atomic<float>* mixParam = nullptr;
    std::atomic<float>* brightnessParam = nullptr;
    std::atomic<float>* outputParam = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PickupSwapProcessor)
};
