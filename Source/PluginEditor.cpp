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

// Pasa una foto en blanco y negro a los azules de la marca (sombras grafito, medios indigo y celeste,
// luces niebla). La marca pide fotos en blanco y negro con un solo plano en azul: se usa en "Hacia".
juce::Image toBrandBlue (const juce::Image& src)
{
    juce::Image out (juce::Image::RGB, src.getWidth(), src.getHeight(), false);
    const juce::Image::BitmapData in (src, juce::Image::BitmapData::readOnly);
    juce::Image::BitmapData dst (out, juce::Image::BitmapData::writeOnly);

    struct Stop { float pos; juce::Colour c; };
    const Stop stops[] = { { 0.0f, colour::grafito }, { 0.45f, colour::indigo }, { 0.75f, colour::celeste }, { 1.0f, colour::niebla } };

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

void drawPanel (juce::Graphics& g, juce::Rectangle<int> r)
{
    g.setColour (colour::linea);
    g.drawRect (r, 1);
}

void drawMonoLabel (juce::Graphics& g, const juce::String& text, juce::Rectangle<int> r,
                    juce::Colour c = colour::gris, juce::Justification j = juce::Justification::centredLeft, bool upper = true)
{
    g.setColour (c);
    g.setFont (Fonts::monoFont (11.0f));
    g.drawText (upper ? text.toUpperCase() : text, r, j, false);
}
} // namespace

//==============================================================================
ArrowButton::ArrowButton (const juce::String& name, bool pointsRight) : juce::Button (name), right (pointsRight)
{
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
    setTooltip (name);
}

void ArrowButton::paintButton (juce::Graphics& g, bool highlighted, bool down)
{
    const auto r = getLocalBounds().toFloat().reduced (1.0f);
    if (highlighted || down)
    {
        g.setGradientFill (brandGradient (r));
        g.fillEllipse (r);
    }
    else
    {
        g.setColour (colour::niebla.withAlpha (0.28f));
        g.drawEllipse (r, 1.0f);
    }

    const auto c = r.getCentre();
    const float s = r.getWidth() * 0.14f;
    juce::Path chevron;
    if (right)
    {
        chevron.startNewSubPath (c.x - s * 0.6f, c.y - s * 1.2f);
        chevron.lineTo (c.x + s * 0.6f, c.y);
        chevron.lineTo (c.x - s * 0.6f, c.y + s * 1.2f);
    }
    else
    {
        chevron.startNewSubPath (c.x + s * 0.6f, c.y - s * 1.2f);
        chevron.lineTo (c.x - s * 0.6f, c.y);
        chevron.lineTo (c.x + s * 0.6f, c.y + s * 1.2f);
    }
    g.setColour (colour::niebla);
    g.strokePath (chevron, juce::PathStrokeType (1.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

//==============================================================================
PickupCard::PickupCard (juce::RangedAudioParameter& param, const juce::String& labelText, bool blueTone)
    : parameter (param),
      attachment (param, [this] (float v) { index = juce::jlimit (0, pickup::numModels() - 1, (int) std::round (v)); repaint(); }),
      label (labelText),
      blue (blueTone)
{
    for (size_t i = 0; i < photos.size(); ++i)
    {
        auto img = loadPhoto ((int) i);
        photos[i] = blue ? toBrandBlue (img) : img;
    }

    prev.onClick = [this] { setIndex ((index + pickup::numModels() - 1) % pickup::numModels()); };
    next.onClick = [this] { setIndex ((index + 1) % pickup::numModels()); };
    addAndMakeVisible (prev);
    addAndMakeVisible (next);

    setMouseCursor (juce::MouseCursor::PointingHandCursor);
    setTitle (label);
    attachment.sendInitialUpdate();
}

void PickupCard::setIndex (int newIndex)
{
    attachment.setValueAsCompleteGesture ((float) newIndex);
}

juce::Rectangle<int> PickupCard::nameArea() const
{
    auto r = getLocalBounds();
    r.removeFromTop (juce::roundToInt (getWidth() / 1.6f));
    return r.reduced (18, 14).withTrimmedRight (80);
}

void PickupCard::paint (juce::Graphics& g)
{
    auto r = getLocalBounds();
    const auto photoArea = r.removeFromTop (juce::roundToInt (getWidth() / 1.6f));

    g.setColour (colour::grafito);
    g.fillRect (getLocalBounds());

    // Foto de la capsula
    const auto& photo = photos[(size_t) kPhotoForModel[(size_t) index]];
    g.drawImage (photo, photoArea.toFloat(), juce::RectanglePlacement::fillDestination);

    // Etiqueta sobre la foto
    auto tag = photoArea.reduced (14).removeFromTop (22);
    const juce::String tagText = label;
    const int tagWidth = juce::roundToInt (juce::GlyphArrangement::getStringWidth (Fonts::monoFont (11.0f, true), tagText)) + 18;
    auto tagBox = tag.removeFromLeft (tagWidth);
    g.setColour (colour::grafito.withAlpha (0.86f));
    g.fillRect (tagBox);
    g.setColour (colour::niebla);
    g.setFont (Fonts::monoFont (11.0f, true));
    g.drawText (tagText, tagBox, juce::Justification::centred, false);

    // Numero del modelo, en mono
    auto numBox = photoArea.reduced (14).removeFromTop (22).removeFromRight (54);
    g.setColour (colour::grafito.withAlpha (0.86f));
    g.fillRect (numBox);
    g.setColour (colour::niebla);
    g.drawText (juce::String (index + 1).paddedLeft ('0', 2) + " / " + juce::String (pickup::numModels()),
                numBox, juce::Justification::centred, false);

    // Texto
    auto text = nameArea();
    const bool hover = isMouseOver (true) && nameArea().contains (getMouseXYRelative());
    g.setColour (colour::niebla);
    g.setFont (Fonts::bodyFont (19.0f, true));
    auto nameLine = text.removeFromTop (28);
    g.drawText (pickup::models()[(size_t) index].name, nameLine, juce::Justification::centredLeft, true);

    if (hover)
    {
        const float w = juce::jmin ((float) nameLine.getWidth(),
                                    juce::GlyphArrangement::getStringWidth (Fonts::bodyFont (19.0f, true), pickup::models()[(size_t) index].name));
        g.setGradientFill (brandGradient (nameLine.toFloat()));
        g.fillRect ((float) nameLine.getX(), (float) nameLine.getBottom() - 1.0f, w, 1.5f);
    }

    g.setColour (colour::gris);
    g.setFont (Fonts::bodyFont (13.5f));
    g.drawText (u8 (kDescriptions[(size_t) index]), text.removeFromTop (22), juce::Justification::centredLeft, true);

    // Borde fino
    g.setColour (colour::linea);
    g.drawRect (getLocalBounds(), 1);
    g.drawHorizontalLine (photoArea.getBottom(), 0.0f, (float) getWidth());
}

void PickupCard::resized()
{
    auto r = getLocalBounds();
    r.removeFromTop (juce::roundToInt (getWidth() / 1.6f));
    auto buttons = r.reduced (16).removeFromRight (72).withSizeKeepingCentre (72, 32);
    prev.setBounds (buttons.removeFromLeft (32));
    next.setBounds (buttons.removeFromRight (32));
}

void PickupCard::mouseUp (const juce::MouseEvent& e)
{
    if (e.mouseWasClicked())
        showMenu();
}

void PickupCard::showMenu()
{
    juce::PopupMenu menu;
    menu.setLookAndFeel (&getLookAndFeel());

    auto addRange = [&] (const char* section, int from, int to)
    {
        menu.addSectionHeader (section);
        for (int i = from; i <= to; ++i)
            menu.addItem (i + 1, pickup::models()[(size_t) i].name, true, i == index);
    };
    addRange ("Single coil", 0, 3);
    addRange ("Humbucker", 4, 6);
    addRange ("Humbucker en split", 7, 9);

    juce::Component::SafePointer<PickupCard> safe (this);
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
    auto r = getLocalBounds().toFloat();
    auto header = r.removeFromTop (30.0f);
    auto plot = r.reduced (14.0f, 6.0f).withTrimmedBottom (18.0f).withTrimmedLeft (22.0f);

    constexpr float range = 15.0f; // +/- dB
    auto xFor = [&] (double f) { return plot.getX() + plot.getWidth() * (float) (std::log10 (f / 20.0) / 3.0); };
    auto yFor = [&] (float db) { return plot.getCentreY() - plot.getHeight() * 0.5f * juce::jlimit (-1.0f, 1.0f, db / range); };

    // Rejilla de linea fina
    g.setFont (Fonts::monoFont (10.0f));
    for (double f : { 100.0, 1000.0, 10000.0 })
    {
        const float x = xFor (f);
        g.setColour (colour::linea);
        g.drawVerticalLine (juce::roundToInt (x), plot.getY(), plot.getBottom());
        g.setColour (colour::gris);
        g.drawText (f >= 1000.0 ? juce::String ((int) (f / 1000.0)) + "K" : juce::String ((int) f),
                    juce::Rectangle<float> (x - 20.0f, plot.getBottom() + 2.0f, 40.0f, 14.0f), juce::Justification::centred, false);
    }
    for (float db : { -12.0f, 0.0f, 12.0f })
    {
        const float y = yFor (db);
        g.setColour (db == 0.0f ? colour::niebla.withAlpha (0.28f) : colour::linea);
        g.drawHorizontalLine (juce::roundToInt (y), plot.getX(), plot.getRight());
        g.setColour (colour::gris);
        g.drawText ((db > 0 ? "+" : "") + juce::String ((int) db), juce::Rectangle<float> (plot.getX() - 30.0f, y - 7.0f, 26.0f, 14.0f),
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

    juce::ColourGradient fillGrad (colour::celeste.withAlpha (0.28f), plot.getBottomLeft(), colour::indigo.withAlpha (0.28f), plot.getTopRight(), false);
    g.setGradientFill (fillGrad);
    g.fillPath (fillPath);
    g.setGradientFill (brandGradient (plot));
    g.strokePath (curvePath, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Encabezado: titulo y el cambio mas grande
    auto h = header.reduced (14.0f, 0.0f);
    drawMonoLabel (g, "Respuesta", h.toNearestInt());
    juce::String peakText = "Sin cambios";
    if (std::abs (peakDb) >= 0.1f)
        peakText = (peakDb > 0 ? "+" : "") + juce::String (peakDb, 1) + " dB  @  "
                   + (peakF >= 1000.0 ? juce::String (peakF / 1000.0, 1) + " kHz" : juce::String ((int) peakF) + " Hz");
    drawMonoLabel (g, peakText, h.toNearestInt(), colour::niebla, juce::Justification::centredRight, false);
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
    drawMonoLabel (g, label, getLocalBounds().removeFromBottom (20), colour::gris, juce::Justification::centred);
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
      fromCard (*p.apvts.getParameter ("desde"), quadro::u8 ("DESDE · TU GUITARRA"), false),
      toCard (*p.apvts.getParameter ("hacia"), quadro::u8 ("HACIA · LO QUE QUIERES OÍR"), true),
      curve (p.apvts),
      mixKnob (p.apvts, "mezcla", "Mezcla", " %"),
      brightnessKnob (p.apvts, "brillo", "Brillo", " dB"),
      outputKnob (p.apvts, "salida", "Salida", " dB")
{
    setLookAndFeel (&lnf);
    isotipo = juce::Drawable::createFromImageData (BinaryData::quadroisotipo_svg, BinaryData::quadroisotipo_svgSize);

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
    g.fillAll (colour::grafito);

    // Encabezado: isotipo, nombre del producto y datos en mono
    auto h = headerArea;
    if (isotipo != nullptr)
        isotipo->drawWithin (g, h.removeFromLeft (30).withSizeKeepingCentre (30, 30).toFloat(), juce::RectanglePlacement::centred, 1.0f);
    h.removeFromLeft (14);
    g.setColour (colour::niebla);
    g.setFont (Fonts::titleFont (22.0f));
    const juce::String title = "PICKUP SWAP";
    const int titleW = juce::roundToInt (juce::GlyphArrangement::getStringWidth (Fonts::titleFont (22.0f), title)) + 4;
    g.drawText (title, h.removeFromLeft (titleW), juce::Justification::centredLeft, false);
    h.removeFromLeft (16);
    g.setColour (colour::linea);
    g.fillRect (h.removeFromLeft (1).withSizeKeepingCentre (1, 22));
    h.removeFromLeft (16);
    drawMonoLabel (g, u8 ("Quadro · Laboratorio creativo digital"), h.removeFromLeft (320));
    drawMonoLabel (g, u8 ("v" JucePlugin_VersionString "  ·  SIN LATENCIA"), h, colour::gris, juce::Justification::centredRight, false);

    g.setColour (colour::linea);
    g.drawHorizontalLine (headerArea.getBottom() + 12, (float) headerArea.getX(), (float) headerArea.getRight());

    // Flecha entre las tarjetas: circulo con el degradado de marca
    const auto c = juce::Rectangle<int> (fromCard.getRight(), fromCard.getY(), toCard.getX() - fromCard.getRight(), fromCard.getWidth() / 1.6)
                       .getCentre().toFloat();
    const auto circle = juce::Rectangle<float> (46.0f, 46.0f).withCentre (c);
    g.setColour (colour::linea);
    g.drawHorizontalLine (juce::roundToInt (c.y), (float) fromCard.getRight(), (float) toCard.getX());
    g.setGradientFill (brandGradient (circle));
    g.fillEllipse (circle);
    juce::Path arrow;
    arrow.startNewSubPath (c.x - 9.0f, c.y);
    arrow.lineTo (c.x + 9.0f, c.y);
    arrow.startNewSubPath (c.x + 3.0f, c.y - 6.0f);
    arrow.lineTo (c.x + 9.0f, c.y);
    arrow.lineTo (c.x + 3.0f, c.y + 6.0f);
    g.setColour (colour::niebla);
    g.strokePath (arrow, juce::PathStrokeType (1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Paneles de abajo
    drawPanel (g, curvePanel);
    drawPanel (g, knobsPanel);
    drawMonoLabel (g, "Ajustes", knobsPanel.reduced (14, 0).removeFromTop (30));

    // Pie
    drawMonoLabel (g, u8 ("Quadro — Pickup Swap"), footerArea);
    drawMonoLabel (g, juce::CharPointer_UTF8 ("33\xc2\xb0" "26\xe2\x80\xb2S 70\xc2\xb0" "39\xe2\x80\xb2O"), footerArea, colour::gris,
                   juce::Justification::centredRight);
}

void PickupSwapEditor::resized()
{
    auto r = getLocalBounds().reduced (24, 0);
    r.removeFromTop (20);
    headerArea = r.removeFromTop (36);
    r.removeFromTop (30);

    cardsArea = r.removeFromTop (296);
    auto cards = cardsArea;
    const int cardW = 352;
    fromCard.setBounds (cards.removeFromLeft (cardW));
    toCard.setBounds (cards.removeFromRight (cardW));

    r.removeFromTop (20);
    footerArea = r.removeFromBottom (40);
    bottomArea = r;
    auto b = bottomArea;
    knobsPanel = b.removeFromRight (300);
    b.removeFromRight (20);
    curvePanel = b;
    curve.setBounds (curvePanel);

    auto k = knobsPanel.reduced (6, 0);
    k.removeFromTop (30);
    k.removeFromBottom (6);
    const int kw = k.getWidth() / 3;
    mixKnob.setBounds (k.removeFromLeft (kw));
    brightnessKnob.setBounds (k.removeFromLeft (kw));
    outputKnob.setBounds (k);
}
