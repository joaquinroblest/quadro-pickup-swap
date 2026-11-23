#pragma once

// Estilo visual de Quadro (brandboard azul): grafito de fondo, niebla para el texto,
// degradado de celeste a indigo para los acentos, lineas finas y tipografias de marca.

#include <juce_gui_basics/juce_gui_basics.h>

#include "BinaryData.h"

namespace quadro
{
// Los textos del codigo estan en UTF-8 (tildes, punto medio, raya).
inline juce::String u8 (const char* text) { return juce::String::fromUTF8 (text); }

namespace colour
{
    const juce::Colour grafito { 0xff16171a };
    const juce::Colour acero   { 0xff2a2c31 };
    const juce::Colour gris    { 0xff8c9099 };
    const juce::Colour niebla  { 0xfff2f3f5 };
    const juce::Colour celeste { 0xff3797f6 };
    const juce::Colour indigo  { 0xff333bf6 };
    const juce::Colour linea   = niebla.withAlpha (0.14f); // bordes y lineas finas
}

// Degradado de marca a 45 grados (celeste abajo a la izquierda, indigo arriba a la derecha).
inline juce::ColourGradient brandGradient (juce::Rectangle<float> r)
{
    return juce::ColourGradient (colour::celeste, r.getBottomLeft(), colour::indigo, r.getTopRight(), false);
}

struct Fonts
{
    juce::Typeface::Ptr title, body, bodyMedium, mono, monoMedium;

    Fonts()
    {
        title      = juce::Typeface::createSystemTypefaceFor (BinaryData::ArchivoExpandedExtraBold_ttf, BinaryData::ArchivoExpandedExtraBold_ttfSize);
        body       = juce::Typeface::createSystemTypefaceFor (BinaryData::StackSansTextRegular_ttf, BinaryData::StackSansTextRegular_ttfSize);
        bodyMedium = juce::Typeface::createSystemTypefaceFor (BinaryData::StackSansTextMedium_ttf, BinaryData::StackSansTextMedium_ttfSize);
        mono       = juce::Typeface::createSystemTypefaceFor (BinaryData::IBMPlexMonoRegular_ttf, BinaryData::IBMPlexMonoRegular_ttfSize);
        monoMedium = juce::Typeface::createSystemTypefaceFor (BinaryData::IBMPlexMonoMedium_ttf, BinaryData::IBMPlexMonoMedium_ttfSize);
    }

    static const Fonts& get()
    {
        static const Fonts f;
        return f;
    }

    static juce::Font make (juce::Typeface::Ptr tf, float height, float tracking = 0.0f)
    {
        return juce::Font (juce::FontOptions (tf).withHeight (height).withKerningFactor (tracking));
    }

    // Titulos: Archivo Expanded
    static juce::Font titleFont (float h) { return make (get().title, h, 0.02f); }
    // Textos: Stack Sans Text
    static juce::Font bodyFont (float h, bool medium = false) { return make (medium ? get().bodyMedium : get().body, h); }
    // Etiquetas y datos: IBM Plex Mono, en mayusculas con un poco de aire
    static juce::Font monoFont (float h, bool medium = false) { return make (medium ? get().monoMedium : get().mono, h, 0.08f); }
};

class LookAndFeel : public juce::LookAndFeel_V4
{
public:
    LookAndFeel()
    {
        setColour (juce::ResizableWindow::backgroundColourId, colour::grafito);
        setColour (juce::PopupMenu::backgroundColourId, colour::acero);
        setColour (juce::PopupMenu::textColourId, colour::niebla);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, colour::celeste.withAlpha (0.22f));
        setColour (juce::PopupMenu::highlightedTextColourId, colour::niebla);
        setColour (juce::TooltipWindow::backgroundColourId, colour::acero);
        setColour (juce::TooltipWindow::textColourId, colour::niebla);
    }

    juce::Font getPopupMenuFont() override { return Fonts::bodyFont (15.0f); }

    void drawPopupMenuBackground (juce::Graphics& g, int width, int height) override
    {
        g.fillAll (colour::acero);
        g.setColour (colour::linea);
        g.drawRect (0, 0, width, height, 1);
    }

    void drawPopupMenuSectionHeader (juce::Graphics& g, const juce::Rectangle<int>& area, const juce::String& sectionName) override
    {
        g.setFont (Fonts::monoFont (11.0f, true));
        g.setColour (colour::gris);
        g.drawText (sectionName.toUpperCase(), area.reduced (14, 0), juce::Justification::bottomLeft, false);
    }

    // Perillas: anillo de linea fina, arco con el degradado de marca y un punto al final.
    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height, float pos,
                           float startAngle, float endAngle, juce::Slider& slider) override
    {
        const auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height).reduced (10.0f);
        const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) / 2.0f;
        const auto centre = bounds.getCentre();
        const float arcRadius = radius - 4.0f;

        // Puntos de escala alrededor (el "universo" de la marca: puntos y orbitas de linea fina)
        g.setColour (colour::gris.withAlpha (0.55f));
        for (int i = 0; i <= 10; ++i)
        {
            const float a = startAngle + (endAngle - startAngle) * (float) i / 10.0f;
            const auto p = centre.getPointOnCircumference (radius + 4.0f, a);
            g.fillEllipse (p.x - 1.0f, p.y - 1.0f, 2.0f, 2.0f);
        }

        // Anillo base
        g.setColour (colour::linea);
        g.drawEllipse (centre.x - arcRadius, centre.y - arcRadius, arcRadius * 2.0f, arcRadius * 2.0f, 1.0f);

        // Arco de valor: en controles con cero al medio (Brillo, Salida) parte desde el centro.
        const double minV = slider.getMinimum(), maxV = slider.getMaximum();
        float fromAngle = startAngle;
        if (minV < 0.0 && maxV > 0.0)
            fromAngle = startAngle + (endAngle - startAngle) * (float) slider.valueToProportionOfLength (0.0);
        const float toAngle = startAngle + pos * (endAngle - startAngle);

        juce::Path arc;
        arc.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f,
                           juce::jmin (fromAngle, toAngle), juce::jmax (fromAngle, toAngle), true);
        g.setGradientFill (brandGradient (bounds));
        g.strokePath (arc, juce::PathStrokeType (2.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Punto indicador
        const auto dot = centre.getPointOnCircumference (arcRadius, toAngle);
        g.setColour (colour::niebla);
        g.fillEllipse (dot.x - 4.0f, dot.y - 4.0f, 8.0f, 8.0f);

        // Valor al centro
        g.setColour (colour::niebla);
        g.setFont (Fonts::monoFont (13.0f, true));
        g.drawText (slider.getTextFromValue (slider.getValue()), bounds.withSizeKeepingCentre (bounds.getWidth(), 20.0f),
                    juce::Justification::centred, false);
    }
};

} // namespace quadro
