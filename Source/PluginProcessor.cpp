#include "PluginProcessor.h"

namespace
{
const juce::String idFrom = "desde";
const juce::String idTo = "hacia";
const juce::String idMix = "mezcla";
const juce::String idBrightness = "brillo";
const juce::String idOutput = "salida";

constexpr int kDefaultFrom = 9; // Split de humbucker potente
constexpr int kDefaultTo = 0;   // Strato vintage

juce::StringArray modelNames()
{
    juce::StringArray names;
    for (const auto& m : pickup::models())
        names.add (m.name);
    return names;
}
} // namespace

PickupSwapProcessor::PickupSwapProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createLayout())
{
    fromParam = apvts.getRawParameterValue (idFrom);
    toParam = apvts.getRawParameterValue (idTo);
    mixParam = apvts.getRawParameterValue (idMix);
    brightnessParam = apvts.getRawParameterValue (idBrightness);
    outputParam = apvts.getRawParameterValue (idOutput);
}

juce::AudioProcessorValueTreeState::ParameterLayout PickupSwapProcessor::createLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { idFrom, 1 }, "Desde", modelNames(), kDefaultFrom));

    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { idTo, 1 }, "Hacia", modelNames(), kDefaultTo));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { idMix, 1 }, "Mezcla",
        juce::NormalisableRange<float> (0.0f, 100.0f, 1.0f), 100.0f,
        juce::AudioParameterFloatAttributes().withLabel ("%")));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { idBrightness, 1 }, "Brillo",
        juce::NormalisableRange<float> (-6.0f, 6.0f, 0.1f), 0.0f,
        juce::AudioParameterFloatAttributes().withLabel ("dB")));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { idOutput, 1 }, "Salida",
        juce::NormalisableRange<float> (-12.0f, 12.0f, 0.1f), 0.0f,
        juce::AudioParameterFloatAttributes().withLabel ("dB")));

    return { params.begin(), params.end() };
}

bool PickupSwapProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto in = layouts.getMainInputChannelSet();
    const auto out = layouts.getMainOutputChannelSet();

    if (in != out)
        return false;

    return in == juce::AudioChannelSet::mono() || in == juce::AudioChannelSet::stereo();
}

void PickupSwapProcessor::prepareToPlay (double newSampleRate, int)
{
    sampleRate = newSampleRate > 0.0 ? newSampleRate : 48000.0;
    fadeLength = juce::jmax (1, (int) std::round (0.02 * sampleRate)); // 20 ms al cambiar de modelo
    fadeRemaining = 0;

    for (auto& c : current)
        c.reset();
    for (auto& c : previous)
        c.reset();

    mixSmoothed.reset (sampleRate, 0.03);
    gainSmoothed.reset (sampleRate, 0.03);
    mixSmoothed.setCurrentAndTargetValue (mixParam->load() / 100.0f);
    gainSmoothed.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (outputParam->load()));

    updateFilters (true);
}

void PickupSwapProcessor::updateFilters (bool force)
{
    const int from = juce::jlimit (0, pickup::numModels() - 1, (int) std::round (fromParam->load()));
    const int to = juce::jlimit (0, pickup::numModels() - 1, (int) std::round (toParam->load()));
    const float brightness = brightnessParam->load();

    const bool modelChanged = from != activeFrom || to != activeTo;

    if (modelChanged || force)
    {
        const auto conv = pickup::conversion (pickup::models()[(size_t) from],
                                              pickup::models()[(size_t) to],
                                              sampleRate);

        if (! force)
        {
            // Mezcla suave entre el filtro anterior y el nuevo para evitar chasquidos.
            previous = current;
            fadeRemaining = fadeLength;
        }

        for (auto& c : current)
            c.conv = conv;

        activeFrom = from;
        activeTo = to;
    }

    if (brightness != activeBrightness || force)
    {
        const auto shelf = pickup::highShelf (sampleRate, kBrightnessFrequency, brightness);

        for (auto& c : current)
            c.shelf = shelf;
        for (auto& c : previous)
            c.shelf = shelf;

        activeBrightness = brightness;
    }
}

void PickupSwapProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();
    const int numChannels = juce::jmin (buffer.getNumChannels(), kMaxChannels);

    for (int ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, numSamples);

    updateFilters (false);

    mixSmoothed.setTargetValue (mixParam->load() / 100.0f);
    gainSmoothed.setTargetValue (juce::Decibels::decibelsToGain (outputParam->load()));

    std::array<float*, kMaxChannels> data {};
    for (int ch = 0; ch < numChannels; ++ch)
        data[(size_t) ch] = buffer.getWritePointer (ch);

    // Se recorre muestra por muestra (y dentro, cada canal) para que las rampas de mezcla,
    // salida y cambio de modelo sean identicas en ambos canales, sin reservar memoria.
    for (int i = 0; i < numSamples; ++i)
    {
        const double mix = mixSmoothed.getNextValue();
        const double gain = gainSmoothed.getNextValue();
        const bool fading = fadeRemaining > 0;
        const double t = fading ? 1.0 - (double) fadeRemaining / (double) fadeLength : 1.0;

        for (int ch = 0; ch < numChannels; ++ch)
        {
            auto* d = data[(size_t) ch];
            const double x = d[i];
            double wet = current[(size_t) ch].process (x);

            if (fading)
            {
                const double old = previous[(size_t) ch].process (x);
                wet = old + t * (wet - old);
            }

            d[i] = (float) ((x + mix * (wet - x)) * gain);
        }

        if (fading)
            --fadeRemaining;
    }
}

juce::AudioProcessorEditor* PickupSwapProcessor::createEditor()
{
    return new juce::GenericAudioProcessorEditor (*this);
}

void PickupSwapProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void PickupSwapProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PickupSwapProcessor();
}
