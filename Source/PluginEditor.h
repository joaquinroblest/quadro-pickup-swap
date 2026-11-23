#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "PluginProcessor.h"
#include "QuadroLookAndFeel.h"

#include <array>
#include <memory>

namespace quadro
{
// Perilla selectora de 10 posiciones (01 a 10), como el selector de un equipo.
// Se gira arrastrando, con la rueda del mouse, con las flechas del teclado,
// o haciendo clic en un numero o en el punto del arco donde se quiere dejar.
class SelectorDial : public juce::Component
{
public:
    explicit SelectorDial (std::function<void (int)> onPick);

    void setIndex (int i) { index = i; repaint(); }
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    bool keyPressed (const juce::KeyPress&) override;

    static juce::Point<float> pointFor (int i, juce::Point<float> centre, float radius);

private:
    int indexAt (juce::Point<float>) const;
    void pick (int i);

    std::function<void (int)> onPick;
    int index = 0, dragStartIndex = 0;
    float wheelAccum = 0.0f;
    bool dragged = false;
};

// Tarjeta "Desde" o "Hacia": etiqueta, contador, selector, foto y nombre del modelo.
class SelectorCard : public juce::Component
{
public:
    SelectorCard (juce::RangedAudioParameter& param, const juce::String& label, bool orangeTone);

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent& e) override
    {
        setMouseCursor (clickArea().contains (e.getPosition()) ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor);
        repaint();
    }
    void mouseExit (const juce::MouseEvent&) override { repaint(); }

private:
    void setIndex (int newIndex);
    void showMenu();
    juce::Rectangle<int> photoArea() const;
    juce::Rectangle<int> textArea() const;
    juce::Rectangle<int> clickArea() const { return photoArea().getUnion (textArea()); }

    juce::ParameterAttachment attachment;
    juce::String label;
    int index = 0;

    SelectorDial dial;
    std::array<juce::Image, 5> photos;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SelectorCard)
};

// Pantalla con lo que hace la conversion: cuanto sube o baja cada frecuencia.
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

    quadro::SelectorCard fromCard, toCard;
    quadro::CurveView curve;
    quadro::Knob mixKnob, brightnessKnob, outputKnob;

    juce::Rectangle<int> bandArea, curvePanel, knobsPanel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PickupSwapEditor)
};
