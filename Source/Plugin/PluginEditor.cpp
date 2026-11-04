#include "Plugin/PluginEditor.h"
#include "Plugin/PluginProcessor.h"
#include "UI/Theme.h"

PrismaEditor::PrismaEditor (PrismaProcessor& p)
    : AudioProcessorEditor (p), display (p), strip (p.getState()), bottom (p.getState())
{
    addAndMakeVisible (display);
    addAndMakeVisible (strip);
    addAndMakeVisible (bottom);
    display.onSelectionChanged = [this] (int b) { strip.setBand (b); strip.resized(); };
    bottom.onRangeChanged = [this] (float r) { display.setRangeDb (r); };
    setResizable (true, true);
    setResizeLimits (760, 420, 2400, 1400);
    setSize (980, 560);
}

void PrismaEditor::paint (juce::Graphics& g)
{
    g.fillAll (prisma::ui::theme::background);
}

void PrismaEditor::resized()
{
    auto r = getLocalBounds();
    bottom.setBounds (r.removeFromBottom (36));
    strip.setBounds (r.removeFromBottom (56));
    display.setBounds (r);
}
