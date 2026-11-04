#include "Plugin/PluginEditor.h"
#include "Plugin/PluginProcessor.h"

PrismaEditor::PrismaEditor (PrismaProcessor& p)
    : AudioProcessorEditor (p), processor (p)
{
    setResizable (true, true);
    setResizeLimits (640, 360, 2400, 1400);
    setSize (980, 560);
}

void PrismaEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff0e1014));
    auto area = getLocalBounds().reduced (24);
    g.setColour (juce::Colour (0xffe9e4d8).withAlpha (0.85f));
    g.setFont (juce::FontOptions (26.0f, juce::Font::plain));
    g.drawText ("Prisma EQ", area.removeFromTop (40), juce::Justification::centredLeft);
    g.setColour (juce::Colour (0xffe9e4d8).withAlpha (0.45f));
    g.setFont (juce::FontOptions (14.0f));
    g.drawText (juce::String ("Version ") + PRISMA_VERSION_STRING + " · moteur d'égalisation en construction",
                area.removeFromTop (24), juce::Justification::centredLeft);
}
