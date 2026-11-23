#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "PluginProcessor.h"
#include "QuadroLookAndFeel.h"

#include <array>
#include <memory>

namespace quadro
{
// Boton redondo de linea fina con una flecha (para pasar de modelo).
class ArrowButton : public juce::Button
{
public:
    ArrowButton (const juce::String& name, bool pointsRight);
    void paintButton (juce::Graphics&, bool highlighted, bool down) override;

private:
    bool right;
};

// Tarjeta de capsula: foto, etiqueta (DESDE / HACIA), nombre del modelo y flechas.
class PickupCard : public juce::Component
{
public:
    PickupCard (juce::RangedAudioParameter& param, const juce::String& label, bool blueTone);
    ~PickupCard() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseEnter (const juce::MouseEvent&) override { repaint(); }
    void mouseExit (const juce::MouseEvent&) override { repaint(); }

private:
    void setIndex (int newIndex);
    void showMenu();
    juce::Rectangle<int> nameArea() const;

    juce::RangedAudioParameter& parameter;
    juce::ParameterAttachment attachment;
    juce::String label;
    bool blue;
    int index = 0;

    ArrowButton prev { "Anterior", false }, next { "Siguiente", true };
    std::array<juce::Image, 5> photos;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PickupCard)
};

// Grafico de lo que hace la conversion: cuanto sube o baja cada frecuencia.
class CurveView : public juce::Component, private juce::Timer
{
public:
    explicit CurveView (juce::AudioProcessorValueTreeState& state);
    void paint (juce::Graphics&) override;

private:
    void timerCallback() override;
    float responseDb (double frequency) const;

    juce::AudioProcessorValueTreeState& apvts;
    std::array<float, 5> lastValues { -1.0f, -1.0f, -1.0f, -1.0f, -1.0f };
    pickup::Biquad conv, shelf;
    double mix = 1.0, outGain = 1.0;
};

// Perilla con su etiqueta debajo.
class Knob : public juce::Component
{
public:
    Knob (juce::AudioProcessorValueTreeState& state, const juce::String& paramId, const juce::String& label, const juce::String& suffix);
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    juce::Slider slider;
    juce::AudioProcessorValueTreeState::SliderAttachment attachment;
    juce::String label;
};
} // namespace quadro

class PickupSwapEditor : public juce::AudioProcessorEditor
{
public:
    explicit PickupSwapEditor (PickupSwapProcessor&);
    ~PickupSwapEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    static constexpr int kWidth = 820;
    static constexpr int kHeight = 620;

private:
    quadro::LookAndFeel lnf;
    std::unique_ptr<juce::Drawable> isotipo;

    quadro::PickupCard fromCard, toCard;
    quadro::CurveView curve;
    quadro::Knob mixKnob, brightnessKnob, outputKnob;

    juce::Rectangle<int> headerArea, cardsArea, bottomArea, curvePanel, knobsPanel, footerArea;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PickupSwapEditor)
};
