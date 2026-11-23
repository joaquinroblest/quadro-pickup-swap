// Crea la interfaz del plugin fuera de un DAW y la guarda como imagen PNG.
//   QuadroUISnapshot salida.png [desde hacia mezcla brillo salida] [escala]
// Sirve para revisar el diseno sin abrir Logic.

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter();

static void setParam (juce::AudioProcessor& p, const juce::String& id, float value)
{
    for (auto* param : p.getParameters())
        if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (param))
            if (ranged->getParameterID() == id)
                ranged->setValueNotifyingHost (ranged->convertTo0to1 (value));
}

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gui;

    const juce::File out (juce::File::getCurrentWorkingDirectory().getChildFile (argc > 1 ? argv[1] : "ui.png"));
    std::unique_ptr<juce::AudioProcessor> proc (createPluginFilter());
    proc->setPlayConfigDetails (2, 2, 48000.0, 512);
    proc->prepareToPlay (48000.0, 512);

    if (argc > 6)
    {
        setParam (*proc, "desde", (float) std::atof (argv[2]));
        setParam (*proc, "hacia", (float) std::atof (argv[3]));
        setParam (*proc, "mezcla", (float) std::atof (argv[4]));
        setParam (*proc, "brillo", (float) std::atof (argv[5]));
        setParam (*proc, "salida", (float) std::atof (argv[6]));
    }
    const float scale = argc > 7 ? (float) std::atof (argv[7]) : 2.0f;

    std::unique_ptr<juce::AudioProcessorEditor> editor (proc->createEditor());

    const auto img = editor->createComponentSnapshot (editor->getLocalBounds(), true, scale);
    out.deleteFile();
    juce::FileOutputStream os (out);
    juce::PNGImageFormat png;
    const bool ok = os.openedOk() && png.writeImageToStream (img, os);
    std::printf ("%s %s (%dx%d)\n", ok ? "Guardado" : "ERROR", out.getFullPathName().toRawUTF8(), img.getWidth(), img.getHeight());

    editor.reset();
    proc.reset();
    return ok ? 0 : 1;
}
