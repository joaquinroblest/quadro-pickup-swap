#pragma once

// Estilo visual de Quadro (brandboard naranjo): negro de fondo, papel para el texto,
// naranjo para simbolos, graficos y acentos, humo para textos secundarios y lineas finas.

#include <juce_gui_basics/juce_gui_basics.h>

#include "BinaryData.h"

namespace quadro
{
// Los textos del codigo estan en UTF-8 (tildes, punto medio, raya).
inline juce::String u8 (const char* text) { return juce::String::fromUTF8 (text); }

namespace colour
{
    const juce::Colour negro           { 0xff131211 }; // fondo principal
    const juce::Colour naranjo         { 0xfff2622e }; // simbolos, graficos y acentos
    const juce::Colour naranjoProfundo { 0xffd24b22 }; // sombras del naranjo
    const juce::Colour papel           { 0xfff6f2eb }; // texto sobre negro
    const juce::Colour humo            { 0xff8e8780 }; // textos secundarios
    const juce::Colour panel           { 0xff1f1d1b }; // menus desplegables (negro levantado)
    const juce::Colour linea           = papel.withAlpha (0.14f); // bordes y lineas finas
}

// Relleno de acento: naranjo plano (la version naranjo de la marca no usa degradados).
inline juce::ColourGradient brandGradient (juce::Rectangle<float> r)
{
    return juce::ColourGradient (colour::naranjo, r.getBottomLeft(), colour::naranjo, r.getTopRight(), false);
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
        setColour (juce::ResizableWindow::backgroundColourId, colour::negro);
        setColour (juce::PopupMenu::backgroundColourId, colour::panel);
        setColour (juce::PopupMenu::textColourId, colour::papel);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, colour::naranjo.withAlpha (0.22f));
        setColour (juce::PopupMenu::highlightedTextColourId, colour::papel);
        setColour (juce::TooltipWindow::backgroundColourId, colour::panel);
        setColour (juce::TooltipWindow::textColourId, colour::papel);
    }

    juce::Font getPopupMenuFont() override { return Fonts::bodyFont (15.0f); }

    void drawPopupMenuBackground (juce::Graphics& g, int width, int height) override
    {
        g.fillAll (colour::panel);
        g.setColour (colour::linea);
        g.drawRect (0, 0, width, height, 1);
    }

    void drawPopupMenuSectionHeader (juce::Graphics& g, const juce::Rectangle<int>& area, const juce::String& sectionName) override
    {
        g.setFont (Fonts::monoFont (11.0f, true));
        g.setColour (colour::humo);
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
        g.setColour (colour::humo.withAlpha (0.55f));
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
        g.setColour (colour::papel);
        g.fillEllipse (dot.x - 4.0f, dot.y - 4.0f, 8.0f, 8.0f);

        // Valor al centro
        g.setColour (colour::papel);
        g.setFont (Fonts::monoFont (13.0f, true));
        g.drawText (slider.getTextFromValue (slider.getValue()), bounds.withSizeKeepingCentre (bounds.getWidth(), 20.0f),
                    juce::Justification::centred, false);
    }
};

} // namespace quadro
