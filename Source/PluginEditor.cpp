#include "PluginEditor.h"

#include <cmath>
#include <complex>

namespace quadro
{
namespace
{
// Foto que corresponde a cada modelo (mismo orden que pickup::models()).
// 0 Strato, 1 Tele, 2 P90, 3 humbucker (grano), 4 humbucker en semitono (split)
constexpr std::array<int, 10> kPhotoForModel { 0, 0, 1, 2, 3, 3, 3, 4, 4, 4 };

const std::array<const char*, 10> kDescriptions {
    "Single coil · la más brillante",
    "Single coil · más salida",
    "Single coil · medios con mordida",
    "Single coil ancha · cálida",
    "Humbucker · clásico",
    "Humbucker · más salida",
    "Humbucker · alta salida, oscuro",
    "Una bobina de humbucker clásico",
    "Una bobina de humbucker moderno",
    "Una bobina de humbucker potente",
};

juce::Image loadPhoto (int i)
{
    switch (i)
    {
        case 0:  return juce::ImageCache::getFromMemory (BinaryData::pickupstrat_jpg, BinaryData::pickupstrat_jpgSize);
        case 1:  return juce::ImageCache::getFromMemory (BinaryData::pickuptele_jpg, BinaryData::pickuptele_jpgSize);
        case 2:  return juce::ImageCache::getFromMemory (BinaryData::pickupp90_jpg, BinaryData::pickupp90_jpgSize);
        case 3:  return juce::ImageCache::getFromMemory (BinaryData::pickuphumbucker_jpg, BinaryData::pickuphumbucker_jpgSize);
        default: return juce::ImageCache::getFromMemory (BinaryData::pickupsplit_jpg, BinaryData::pickupsplit_jpgSize);
    }
}

// Pasa una foto en blanco y negro al naranjo de la marca (sombras negro, medios naranjo profundo
// y naranjo, luces papel). La marca pide fotos en blanco y negro con un solo plano en naranjo: se usa en "Hacia".
juce::Image toBrandTone (const juce::Image& src)
{
    juce::Image out (juce::Image::RGB, src.getWidth(), src.getHeight(), false);
    const juce::Image::BitmapData in (src, juce::Image::BitmapData::readOnly);
    juce::Image::BitmapData dst (out, juce::Image::BitmapData::writeOnly);

    struct Stop { float pos; juce::Colour c; };
    const Stop stops[] = { { 0.0f, colour::negro }, { 0.45f, colour::naranjoProfundo }, { 0.75f, colour::naranjo }, { 1.0f, colour::papel } };

    std::array<juce::Colour, 256> lut;
    for (int v = 0; v < 256; ++v)
    {
        const float t = (float) v / 255.0f;
        int k = 0;
        while (k < 2 && t > stops[k + 1].pos)
            ++k;
        const float local = (t - stops[k].pos) / (stops[k + 1].pos - stops[k].pos);
        lut[(size_t) v] = stops[k].c.interpolatedWith (stops[k + 1].c, juce::jlimit (0.0f, 1.0f, local));
    }

    for (int y = 0; y < src.getHeight(); ++y)
        for (int x = 0; x < src.getWidth(); ++x)
        {
            const auto p = in.getPixelColour (x, y);
            const int lum = juce::jlimit (0, 255, (int) std::round (p.getPerceivedBrightness() * 255.0f));
            dst.setPixelColour (x, y, lut[(size_t) lum]);
        }

    return out;
}

void drawMonoLabel (juce::Graphics& g, const juce::String& text, juce::Rectangle<int> r,
                    juce::Colour c = colour::humo, juce::Justification j = juce::Justification::centredLeft,
                    bool upper = true, bool medium = false, float size = 11.0f)
{
    g.setColour (c);
    g.setFont (Fonts::monoFont (size, medium));
    g.drawText (upper ? text.toUpperCase() : text, r, j, false);
}

// Grupo al que pertenece cada modelo (mismo orden que pickup::models()).
const char* groupFor (int i)
{
    return i <= 3 ? "SINGLE COIL" : (i <= 6 ? "HUMBUCKER" : "HUMBUCKER EN SPLIT");
}

// Texto con saltos de linea a un ancho dado; devuelve la altura que ocupo.
float drawWrapped (juce::Graphics& g, const juce::String& text, const juce::Font& font, juce::Colour c,
                   juce::Rectangle<float> area)
{
    juce::AttributedString s;
    s.setWordWrap (juce::AttributedString::byWord);
    s.setJustification (juce::Justification::topLeft);
    s.append (text, font, c);
    juce::TextLayout layout;
    layout.createLayout (s, area.getWidth());
    layout.draw (g, area.withHeight (layout.getHeight()));
    return layout.getHeight();
}
} // namespace

//==============================================================================
SelectorDial::SelectorDial (std::function<void (int)> cb) : onPick (std::move (cb))
{
    setWantsKeyboardFocus (true);
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
    setTitle ("Selector");
}

juce::Point<float> SelectorDial::pointFor (int i, juce::Point<float> c, float radius)
{
    // 01 abajo a la izquierda, sube por la izquierda, pasa por arriba y baja hasta 10 abajo a la derecha.
    const float a = juce::degreesToRadians (225.0f - 30.0f * (float) i);
    return { c.x + radius * std::cos (a), c.y - radius * std::sin (a) };
}

void SelectorDial::paint (juce::Graphics& g)
{
    const auto c = getLocalBounds().toFloat().getCentre();
    const float r = 66.0f;

    // Numeros alrededor
    g.setFont (Fonts::monoFont (11.0f, true));
    for (int i = 0; i < pickup::numModels(); ++i)
    {
        const auto p = pointFor (i, c, r + 19.0f);
        g.setColour (i == index ? colour::naranjo : colour::humo);
        g.drawText (juce::String (i + 1).paddedLeft ('0', 2), juce::Rectangle<float> (28.0f, 16.0f).withCentre (p),
                    juce::Justification::centred, false);
    }

    // Cuerpo de la perilla: aro de linea fina y un aro interior mas tenue
    g.setColour (juce::Colour (0xff1c1a19));
    g.fillEllipse (juce::Rectangle<float> (r * 2.0f, r * 2.0f).withCentre (c));
    g.setColour (colour::papel.withAlpha (hasKeyboardFocus (false) ? 0.5f : 0.28f));
    g.drawEllipse (juce::Rectangle<float> (r * 2.0f, r * 2.0f).withCentre (c), 1.0f);
    g.setColour (colour::papel.withAlpha (0.10f));
    g.drawEllipse (juce::Rectangle<float> ((r - 24.0f) * 2.0f, (r - 24.0f) * 2.0f).withCentre (c), 1.0f);

    // Indicador naranjo
    const auto tip = pointFor (index, c, r - 14.0f);
    g.setColour (colour::naranjo);
    g.drawLine ({ c, tip }, 3.0f);
    g.fillEllipse (juce::Rectangle<float> (10.0f, 10.0f).withCentre (tip));
}

int SelectorDial::indexAt (juce::Point<float> p) const
{
    const auto c = getLocalBounds().toFloat().getCentre();
    if (p.getDistanceFrom (c) < 12.0f)
        return -1;

    // Angulo desde el centro, medido como en el dibujo (0 grados a la derecha, sube contra reloj)
    float deg = juce::radiansToDegrees (std::atan2 (c.y - p.y, p.x - c.x));
    float t = 225.0f - deg; // 0 en el 01, 270 en el 10
    while (t < -45.0f) t += 360.0f;
    while (t >= 315.0f) t -= 360.0f;
    if (t < -18.0f || t > 288.0f)
        return -1; // zona muerta abajo, entre el 01 y el 10
    return juce::jlimit (0, pickup::numModels() - 1, juce::roundToInt (t / 30.0f));
}

void SelectorDial::pick (int i)
{
    i = juce::jlimit (0, pickup::numModels() - 1, i);
    if (i != index && onPick)
        onPick (i);
}

void SelectorDial::mouseDown (const juce::MouseEvent&)
{
    dragStartIndex = index;
    dragged = false;
}

void SelectorDial::mouseDrag (const juce::MouseEvent& e)
{
    const int dist = e.getDistanceFromDragStartX() - e.getDistanceFromDragStartY();
    if (! dragged && std::abs (dist) < 4)
        return;
    dragged = true;
    pick (dragStartIndex + dist / 22);
}

void SelectorDial::mouseUp (const juce::MouseEvent& e)
{
    if (! dragged)
    {
        const int i = indexAt (e.position);
        if (i >= 0)
            pick (i);
    }
}

void SelectorDial::mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& w)
{
    wheelAccum += (w.isReversed ? -w.deltaY : w.deltaY) + w.deltaX;
    while (wheelAccum >= 0.12f) { wheelAccum -= 0.12f; pick (index + 1); }
    while (wheelAccum <= -0.12f) { wheelAccum += 0.12f; pick (index - 1); }
}

bool SelectorDial::keyPressed (const juce::KeyPress& k)
{
    if (k == juce::KeyPress::rightKey || k == juce::KeyPress::upKey)   { pick (index + 1); return true; }
    if (k == juce::KeyPress::leftKey  || k == juce::KeyPress::downKey) { pick (index - 1); return true; }
    return false;
}

//==============================================================================
SelectorCard::SelectorCard (juce::RangedAudioParameter& param, const juce::String& labelText, bool orangeTone)
    : attachment (param, [this] (float v)
                  {
                      index = juce::jlimit (0, pickup::numModels() - 1, (int) std::round (v));
                      dial.setIndex (index);
                      repaint();
                  }),
      label (labelText),
      dial ([this] (int i) { setIndex (i); })
{
    for (size_t i = 0; i < photos.size(); ++i)
    {
        auto img = loadPhoto ((int) i);
        photos[i] = orangeTone ? toBrandTone (img) : img;
    }

    addAndMakeVisible (dial);
    dial.setTitle (label);
    setTitle (label);
    attachment.sendInitialUpdate();
}

void SelectorCard::setIndex (int newIndex)
{
    attachment.setValueAsCompleteGesture ((float) newIndex);
}

juce::Rectangle<int> SelectorCard::photoArea() const { return { 214, 40, 138, 84 }; }
juce::Rectangle<int> SelectorCard::textArea() const  { return { 214, 134, 142, getHeight() - 134 - 12 }; }

void SelectorCard::paint (juce::Graphics& g)
{
    auto top = getLocalBounds().reduced (14, 0).removeFromTop (24).withY (12);
    drawMonoLabel (g, label, top, colour::humo, juce::Justification::centredLeft, true, true);
    drawMonoLabel (g, juce::String (index + 1).paddedLeft ('0', 2) + " / " + juce::String (pickup::numModels()), top,
                   colour::humo, juce::Justification::centredRight);

    const bool hover = isMouseOver (false) && clickArea().contains (getMouseXYRelative());

    // Foto de la capsula
    const auto pa = photoArea();
    g.drawImage (photos[(size_t) kPhotoForModel[(size_t) index]], pa.toFloat(), juce::RectanglePlacement::fillDestination);
    g.setColour (hover ? colour::naranjo : colour::linea);
    g.drawRect (pa, 1);

    // Grupo, nombre y descripcion
    auto t = textArea().toFloat();
    drawMonoLabel (g, groupFor (index), t.removeFromTop (14.0f).toNearestInt(), colour::humo, juce::Justification::centredLeft, true, false, 10.0f);
    t.removeFromTop (4.0f);
    const float nameH = drawWrapped (g, pickup::models()[(size_t) index].name, Fonts::bodyFont (15.0f, true), colour::papel, t);
    if (hover)
    {
        g.setColour (colour::naranjo);
        g.fillRect (t.getX(), t.getY() + nameH + 1.0f, 28.0f, 1.5f);
    }
    t.removeFromTop (nameH + 6.0f);
    drawWrapped (g, u8 (kDescriptions[(size_t) index]), Fonts::bodyFont (12.0f), colour::humo, t);

    g.setColour (colour::linea);
    g.drawRect (getLocalBounds(), 1);
}

void SelectorCard::resized()
{
    dial.setBounds (2, 34, 216, 216);
}

void SelectorCard::mouseUp (const juce::MouseEvent& e)
{
    if (e.mouseWasClicked() && clickArea().contains (e.getPosition()))
        showMenu();
}

void SelectorCard::showMenu()
{
    juce::PopupMenu menu;
    menu.setLookAndFeel (&getLookAndFeel());

    auto addRange = [&] (const char* section, int from, int to)
    {
        menu.addSectionHeader (section);
        for (int i = from; i <= to; ++i)
            menu.addItem (i + 1, juce::String (i + 1).paddedLeft ('0', 2) + "   " + pickup::models()[(size_t) i].name, true, i == index);
    };
    addRange ("Single coil", 0, 3);
    addRange ("Humbucker", 4, 6);
    addRange ("Humbucker en split", 7, 9);

    juce::Component::SafePointer<SelectorCard> safe (this);
    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this).withMinimumWidth (getWidth()),
                        [safe] (int result)
                        {
                            if (safe != nullptr && result > 0)
                                safe->setIndex (result - 1);
                        });
}

//==============================================================================
CurveView::CurveView (juce::AudioProcessorValueTreeState& state) : apvts (state)
{
    setInterceptsMouseClicks (false, false);
    timerCallback();
    startTimerHz (30);
}

void CurveView::timerCallback()
{
    const std::array<float, 5> values {
        apvts.getRawParameterValue ("desde")->load(),
        apvts.getRawParameterValue ("hacia")->load(),
        apvts.getRawParameterValue ("mezcla")->load(),
        apvts.getRawParameterValue ("brillo")->load(),
        apvts.getRawParameterValue ("salida")->load(),
    };

    if (values == lastValues)
        return;

    lastValues = values;
    const auto& ms = pickup::models();
    const int from = juce::jlimit (0, pickup::numModels() - 1, (int) std::round (values[0]));
    const int to = juce::jlimit (0, pickup::numModels() - 1, (int) std::round (values[1]));
    constexpr double fs = 96000.0; // solo para dibujar: frecuencia alta para que la curva sea fiel hasta 20 kHz
    conv = pickup::conversion (ms[(size_t) from], ms[(size_t) to], fs);
    shelf = pickup::highShelf (fs, 5000.0, values[3]);
    mix = values[2] / 100.0;
    outGain = juce::Decibels::decibelsToGain ((double) values[4]);
    repaint();
}

float CurveView::responseDb (double f) const
{
    constexpr double fs = 96000.0;
    using cd = std::complex<double>;
    const cd z1 = std::polar (1.0, -2.0 * pickup::kPi * f / fs);
    auto h = [&] (const pickup::Biquad& c) { return (c.b0 + c.b1 * z1 + c.b2 * z1 * z1) / (1.0 + c.a1 * z1 + c.a2 * z1 * z1); };
    const cd wet = h (conv) * h (shelf);
    const cd total = ((1.0 - mix) + mix * wet) * outGain;
    return (float) (20.0 * std::log10 (std::max (1.0e-6, std::abs (total))));
}

void CurveView::paint (juce::Graphics& g)
{
    // Pantalla un poco mas oscura que el fondo, como un visor
    g.setColour (juce::Colour (0xff0d0c0c));
    g.fillRect (getLocalBounds());
    g.setColour (colour::linea);
    g.drawRect (getLocalBounds(), 1);

    auto r = getLocalBounds().toFloat();
    const auto header = juce::Rectangle<float> (14.0f, 12.0f, r.getWidth() - 28.0f, 16.0f);
    const auto plot = juce::Rectangle<float> (40.0f, 42.0f, r.getWidth() - 54.0f, r.getHeight() - 42.0f - 32.0f);

    constexpr float range = 15.0f; // +/- dB
    auto xFor = [&] (double f) { return plot.getX() + plot.getWidth() * (float) (std::log10 (f / 20.0) / 3.0); };
    auto yFor = [&] (float db) { return plot.getCentreY() - plot.getHeight() * 0.5f * juce::jlimit (-1.0f, 1.0f, db / range); };

    // Rejilla de linea fina con sus valores
    g.setFont (Fonts::monoFont (10.0f));
    for (double f : { 100.0, 1000.0, 10000.0 })
    {
        const float x = xFor (f);
        g.setColour (colour::linea);
        g.drawVerticalLine (juce::roundToInt (x), plot.getY(), plot.getBottom());
        g.setColour (colour::humo);
        g.drawText (f >= 1000.0 ? juce::String ((int) (f / 1000.0)) + "K" : juce::String ((int) f),
                    juce::Rectangle<float> (x - 20.0f, plot.getBottom() + 6.0f, 40.0f, 14.0f), juce::Justification::centred, false);
    }
    for (float db : { -12.0f, 0.0f, 12.0f })
    {
        const float y = yFor (db);
        g.setColour (colour::linea);
        g.drawHorizontalLine (juce::roundToInt (y), plot.getX(), plot.getRight());
        g.setColour (colour::humo);
        g.drawText ((db > 0 ? "+" : "") + juce::String ((int) db), juce::Rectangle<float> (plot.getX() - 34.0f, y - 7.0f, 28.0f, 14.0f),
                    juce::Justification::centredRight, false);
    }

    // Curva
    juce::Path curvePath, fillPath;
    float peakDb = 0.0f;
    double peakF = 1000.0;
    const int n = juce::jmax (2, (int) plot.getWidth());
    for (int i = 0; i <= n; ++i)
    {
        const double f = 20.0 * std::pow (1000.0, (double) i / n);
        const float db = responseDb (f);
        if (std::abs (db) > std::abs (peakDb)) { peakDb = db; peakF = f; }
        const float x = plot.getX() + plot.getWidth() * (float) i / (float) n;
        const float y = yFor (db);
        if (i == 0) { curvePath.startNewSubPath (x, y); fillPath.startNewSubPath (x, yFor (0.0f)); }
        else curvePath.lineTo (x, y);
        fillPath.lineTo (x, y);
    }
    fillPath.lineTo (plot.getRight(), yFor (0.0f));
    fillPath.closeSubPath();

    g.setColour (colour::naranjo.withAlpha (0.22f));
    g.fillPath (fillPath);
    g.setColour (colour::naranjo);
    g.strokePath (curvePath, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Marca en el punto de mayor cambio
    const bool hasPeak = std::abs (peakDb) >= 0.1f;
    if (hasPeak)
    {
        const juce::Point<float> p (xFor (peakF), yFor (peakDb));
        g.setColour (colour::papel.withAlpha (0.5f));
        g.drawEllipse (juce::Rectangle<float> (22.0f, 22.0f).withCentre (p), 1.0f);
        g.setColour (colour::papel);
        g.fillEllipse (juce::Rectangle<float> (10.0f, 10.0f).withCentre (p));
    }

    // Encabezado: titulo y el cambio mas grande
    drawMonoLabel (g, "Respuesta", header.toNearestInt());
    juce::String peakText = "Sin cambios";
    if (hasPeak)
        peakText = (peakDb > 0 ? "+" : "") + juce::String (peakDb, 1) + " dB  " + u8 ("·") + "  "
                   + (peakF >= 1000.0 ? juce::String (peakF / 1000.0, 1) + " kHz" : juce::String ((int) peakF) + " Hz");
    drawMonoLabel (g, peakText, header.toNearestInt(), hasPeak ? colour::naranjo : colour::humo,
                   juce::Justification::centredRight, false, true);
}

//==============================================================================
Knob::Knob (juce::AudioProcessorValueTreeState& state, const juce::String& paramId, const juce::String& labelText, const juce::String& suffix)
    : attachment (state, paramId, slider), label (labelText)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
    slider.setMouseCursor (juce::MouseCursor::UpDownResizeCursor);
    slider.setDoubleClickReturnValue (true, state.getParameter (paramId)->convertFrom0to1 (state.getParameter (paramId)->getDefaultValue()));
    slider.setTitle (labelText);
    slider.setTextValueSuffix (suffix);
    addAndMakeVisible (slider);
}

void Knob::paint (juce::Graphics& g)
{
    drawMonoLabel (g, label, getLocalBounds().removeFromBottom (20), colour::humo, juce::Justification::centred);
}

void Knob::resized()
{
    auto r = getLocalBounds();
    r.removeFromBottom (22);
    slider.setBounds (r);
}
} // namespace quadro

//==============================================================================
PickupSwapEditor::PickupSwapEditor (PickupSwapProcessor& p)
    : AudioProcessorEditor (p),
      fromCard (*p.apvts.getParameter ("desde"), "DESDE", false),
      toCard (*p.apvts.getParameter ("hacia"), "HACIA", true),
      curve (p.apvts),
      mixKnob (p.apvts, "mezcla", "Mezcla", " %"),
      brightnessKnob (p.apvts, "brillo", "Brillo", " dB"),
      outputKnob (p.apvts, "salida", "Salida", " dB")
{
    setLookAndFeel (&lnf);
    isotipo = juce::Drawable::createFromImageData (BinaryData::quadroisotipo_svg, BinaryData::quadroisotipo_svgSize);
    if (isotipo != nullptr)
        isotipo->replaceColour (quadro::colour::naranjo, quadro::colour::negro); // sobre naranjo, siempre negro

    addAndMakeVisible (fromCard);
    addAndMakeVisible (toCard);
    addAndMakeVisible (curve);
    addAndMakeVisible (mixKnob);
    addAndMakeVisible (brightnessKnob);
    addAndMakeVisible (outputKnob);

    setSize (kWidth, kHeight);
}

PickupSwapEditor::~PickupSwapEditor()
{
    setLookAndFeel (nullptr);
}

void PickupSwapEditor::paint (juce::Graphics& g)
{
    using namespace quadro;
    g.fillAll (colour::negro);

    // Franja naranja de arriba, con todo en negro
    g.setColour (colour::naranjo);
    g.fillRect (bandArea);
    auto h = bandArea.reduced (24, 0);
    if (isotipo != nullptr)
        isotipo->drawWithin (g, h.removeFromLeft (30).withSizeKeepingCentre (30, 30).toFloat(), juce::RectanglePlacement::centred, 1.0f);
    h.removeFromLeft (14);
    g.setColour (colour::negro);
    g.setFont (Fonts::titleFont (22.0f));
    const juce::String title = "PICKUP SWAP";
    const int titleW = juce::roundToInt (juce::GlyphArrangement::getStringWidth (Fonts::titleFont (22.0f), title)) + 4;
    g.drawText (title, h.removeFromLeft (titleW), juce::Justification::centredLeft, false);
    h.removeFromLeft (16);
    g.setColour (colour::negro.withAlpha (0.35f));
    g.fillRect (h.removeFromLeft (1).withSizeKeepingCentre (1, 22));
    h.removeFromLeft (16);
    drawMonoLabel (g, u8 ("SELECTOR DE CÁPSULA"), h.removeFromLeft (220), colour::negro, juce::Justification::centredLeft, false, true);
    drawMonoLabel (g, u8 ("v" JucePlugin_VersionString " · QUADRO · 33°26′S 70°39′O"), h, colour::negro,
                   juce::Justification::centredRight, false, true);

    // Tornillos en las esquinas, como en un panel de equipo
    g.setColour (colour::papel.withAlpha (0.28f));
    for (auto p : { juce::Point<float> (12.0f, 95.0f), juce::Point<float> ((float) getWidth() - 12.0f, 95.0f),
                    juce::Point<float> (12.0f, (float) getHeight() - 13.0f), juce::Point<float> ((float) getWidth() - 12.0f, (float) getHeight() - 13.0f) })
        g.drawEllipse (juce::Rectangle<float> (10.0f, 10.0f).withCentre (p), 1.0f);

    // Flecha entre las dos tarjetas
    const auto c = juce::Point<float> ((float) (fromCard.getRight() + toCard.getX()) * 0.5f, (float) fromCard.getBounds().getCentreY());
    const auto circle = juce::Rectangle<float> (38.0f, 38.0f).withCentre (c);
    g.setColour (colour::naranjo);
    g.fillEllipse (circle);
    juce::Path arrow;
    arrow.startNewSubPath (c.x - 7.0f, c.y);
    arrow.lineTo (c.x + 7.0f, c.y);
    arrow.startNewSubPath (c.x + 2.0f, c.y - 5.0f);
    arrow.lineTo (c.x + 7.0f, c.y);
    arrow.lineTo (c.x + 2.0f, c.y + 5.0f);
    g.setColour (colour::negro);
    g.strokePath (arrow, juce::PathStrokeType (1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Panel de perillas
    g.setColour (colour::linea);
    g.drawRect (knobsPanel, 1);
}

void PickupSwapEditor::resized()
{
    bandArea = getLocalBounds().removeFromTop (80);

    fromCard.setBounds (24, 112, 366, 258);
    toCard.setBounds (430, 112, 366, 258);

    curvePanel = { 24, 390, 440, 200 };
    knobsPanel = { 484, 390, 312, 200 };
    curve.setBounds (curvePanel);

    auto k = knobsPanel.reduced (0, 24);
    const int kw = (k.getWidth() - 12) / 3;
    k.removeFromLeft (6);
    mixKnob.setBounds (k.removeFromLeft (kw));
    brightnessKnob.setBounds (k.removeFromLeft (kw));
    outputKnob.setBounds (k.removeFromLeft (kw));
}
