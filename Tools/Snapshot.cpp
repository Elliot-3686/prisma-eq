// Capture PNG de l'interface, pour le contrôle visuel en CI : instancie le processeur
// et l'éditeur hors écran avec quelques bandes de démonstration, puis écrit l'image.
// Usage : "Prisma Snapshot" [fichier.png]

#include "Plugin/PluginProcessor.h"
#include "Plugin/PluginEditor.h"

#include <cstdio>

int main (int argc, char* argv[])
{
    juce::ScopedJuceInitialiser_GUI gui;
    const auto file = juce::File::getCurrentWorkingDirectory().getChildFile (argc > 1 ? argv[1] : "interface.png");

    auto proc = std::make_unique<PrismaProcessor>();
    proc->setPlayConfigDetails (2, 2, 48000.0, 512);
    proc->prepareToPlay (48000.0, 512);

    using prisma::dsp::Shape;
    using prisma::dsp::Placement;
    struct Demo { Shape shape; float freq, gain, q, slope; Placement place; };
    const Demo demo[] = {
        { Shape::LowCut,     30.0f,  0.0f, 0.71f, 24.0f, Placement::Stereo },
        { Shape::LowShelf,  120.0f,  3.5f, 0.71f, 12.0f, Placement::Stereo },
        { Shape::Bell,      450.0f, -4.5f, 1.40f, 12.0f, Placement::Stereo },
        { Shape::Bell,     2500.0f,  3.0f, 0.90f, 12.0f, Placement::Mid },
        { Shape::Notch,    6000.0f,  0.0f, 6.00f, 12.0f, Placement::Stereo },
        { Shape::HighShelf,10000.0f, 2.5f, 0.71f, 12.0f, Placement::Side },
        { Shape::HighCut,  18000.0f, 0.0f, 0.71f, 18.0f, Placement::Stereo },
    };

    auto& state = proc->getState();
    auto set = [&state] (int band, const char* name, float v)
    {
        if (auto* p = state.getParameter (prisma::params::bandId (band, name)))
            p->setValueNotifyingHost (p->convertTo0to1 (v));
    };
    for (int b = 0; b < (int) std::size (demo); ++b)
    {
        const auto& d = demo[b];
        set (b, prisma::params::id::shape, (float) (int) d.shape);
        set (b, prisma::params::id::freq, d.freq);
        set (b, prisma::params::id::gain, d.gain);
        set (b, prisma::params::id::q, d.q);
        set (b, prisma::params::id::slope, d.slope);
        set (b, prisma::params::id::placement, (float) (int) d.place);
        set (b, prisma::params::id::enabled, 1.0f);
        set (b, prisma::params::id::used, 1.0f);
    }

    std::unique_ptr<juce::AudioProcessorEditor> editor (proc->createEditor());
    editor->setSize (1100, 620);
    if (auto* e = dynamic_cast<PrismaEditor*> (editor.get()))
        e->selectBand (2);

    const auto image = editor->createComponentSnapshot (editor->getLocalBounds(), true, 2.0f);
    file.deleteFile();
    bool ok = false;
    {
        juce::FileOutputStream stream (file);
        juce::PNGImageFormat png;
        ok = stream.openedOk() && png.writeImageToStream (image, stream);
    }
    editor.reset();   // l'éditeur doit disparaître avant le processeur
    proc.reset();
    std::printf ("%s : %s (%d x %d)\n", ok ? "Capture écrite" : "ÉCHEC de la capture",
                 file.getFullPathName().toRawUTF8(), image.getWidth(), image.getHeight());
    return ok ? 0 : 1;
}
