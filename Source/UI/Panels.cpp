#include "UI/Panels.h"
#include "UI/Strings_fr.h"
#include "UI/Theme.h"
#include "Params/ParameterLayout.h"

namespace prisma::ui
{
    namespace
    {
        void styleSlider (juce::Slider& s, juce::Label& l, const juce::String& name, juce::Component& parent)
        {
            s.setSliderStyle (juce::Slider::LinearBar);
            s.setTextBoxIsEditable (true);
            s.setColour (juce::Slider::trackColourId, theme::gridStrong);
            s.setColour (juce::Slider::textBoxTextColourId, theme::text);
            l.setText (name, juce::dontSendNotification);
            l.setColour (juce::Label::textColourId, theme::textDim);
            l.setJustificationType (juce::Justification::centredLeft);
            parent.addAndMakeVisible (s);
            parent.addAndMakeVisible (l);
        }
    }

    BandStrip::BandStrip (juce::AudioProcessorValueTreeState& s) : state (s)
    {
        title.setColour (juce::Label::textColourId, theme::text);
        title.setFont (juce::FontOptions (15.0f, juce::Font::bold));
        addAndMakeVisible (title);
        shape.addItemList (params::shapeNames(), 1);
        placement.addItemList (params::placementNames(), 1);
        for (auto* c : { &shape, &placement }) addAndMakeVisible (*c);
        styleSlider (freq, freqL, str::freq, *this);
        styleSlider (gain, gainL, str::gain, *this);
        styleSlider (q, qL, str::q, *this);
        styleSlider (slope, slopeL, str::slope, *this);
        active.setButtonText (str::active);
        gainQ.setButtonText (str::gainQ);
        addAndMakeVisible (active);
        addAndMakeVisible (gainQ);
        setBand (-1);
    }

    void BandStrip::setBand (int b)
    {
        band = b;
        freqA.reset(); gainA.reset(); qA.reset(); slopeA.reset();
        shapeA.reset(); placementA.reset(); activeA.reset(); gainQA.reset();
        const bool has = b >= 0;
        for (auto* c : std::initializer_list<juce::Component*> { &shape, &placement, &freq, &gain, &q, &slope,
                                                                 &freqL, &gainL, &qL, &slopeL, &active, &gainQ })
            c->setVisible (has);
        if (! has)
        {
            title.setText (str::noBand, juce::dontSendNotification);
            title.setFont (juce::FontOptions (13.0f));
            return;
        }
        title.setText (str::band + " " + juce::String (b + 1), juce::dontSendNotification);
        title.setFont (juce::FontOptions (15.0f, juce::Font::bold));
        title.setColour (juce::Label::textColourId, theme::bandColour (b));
        auto id = [b] (const char* n) { return params::bandId (b, n); };
        freqA = std::make_unique<SliderAtt> (state, id (params::id::freq), freq);
        gainA = std::make_unique<SliderAtt> (state, id (params::id::gain), gain);
        qA = std::make_unique<SliderAtt> (state, id (params::id::q), q);
        slopeA = std::make_unique<SliderAtt> (state, id (params::id::slope), slope);
        shapeA = std::make_unique<ComboAtt> (state, id (params::id::shape), shape);
        placementA = std::make_unique<ComboAtt> (state, id (params::id::placement), placement);
        activeA = std::make_unique<ButtonAtt> (state, id (params::id::enabled), active);
        gainQA = std::make_unique<ButtonAtt> (state, id (params::id::gainQ), gainQ);
    }

    void BandStrip::paint (juce::Graphics& g) { g.fillAll (theme::panel); }

    void BandStrip::resized()
    {
        auto r = getLocalBounds().reduced (10, 6);
        if (band < 0) { title.setBounds (r); return; }
        title.setBounds (r.removeFromLeft (80));
        auto col = [&r] (int w) { auto c = r.removeFromLeft (w); r.removeFromLeft (8); return c; };
        auto pair = [] (juce::Rectangle<int> c, juce::Label& l, juce::Component& comp)
        {
            l.setBounds (c.removeFromTop (c.getHeight() / 2));
            comp.setBounds (c);
        };
        auto c1 = col (130); shape.setBounds (c1.removeFromTop (c1.getHeight() / 2).reduced (0, 2)); placement.setBounds (c1.reduced (0, 2));
        pair (col (120), freqL, freq);
        pair (col (100), gainL, gain);
        pair (col (90), qL, q);
        pair (col (100), slopeL, slope);
        auto c6 = col (90); active.setBounds (c6.removeFromTop (c6.getHeight() / 2)); gainQ.setBounds (c6);
    }

    BottomBar::BottomBar (juce::AudioProcessorValueTreeState& s)
    {
        mode.addItemList ({ "Zero Latency", "Natural Phase", "Linear Phase" }, 1);
        mode.setTooltip (str::modeNotYet);
        modeA = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (s, params::id::mode, mode);
        range.addItemList ({ juce::String::fromUTF8 ("±3 dB"), juce::String::fromUTF8 ("±6 dB"), juce::String::fromUTF8 ("±12 dB"), juce::String::fromUTF8 ("±30 dB") }, 1);
        range.setSelectedId (3, juce::dontSendNotification);
        range.onChange = [this]
        {
            static constexpr float r[] = { 3.0f, 6.0f, 12.0f, 30.0f };
            if (onRangeChanged) onRangeChanged (r[juce::jlimit (0, 3, range.getSelectedItemIndex())]);
        };
        for (auto* c : { &mode, &range }) addAndMakeVisible (*c);
        styleSlider (outGain, outL, str::output, *this);
        styleSlider (gainScale, scaleL, str::gainScale, *this);
        outA = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (s, params::id::outGain, outGain);
        scaleA = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (s, params::id::gainScale, gainScale);
        bypass.setButtonText (str::bypass);
        autoGain.setButtonText (str::autoGain);
        invert.setButtonText (str::phase);
        for (auto* b : { &bypass, &autoGain, &invert }) addAndMakeVisible (*b);
        bypassA = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (s, params::id::bypass, bypass);
        autoA = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (s, params::id::autoGain, autoGain);
        invertA = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (s, params::id::phaseInvert, invert);
    }

    void BottomBar::paint (juce::Graphics& g) { g.fillAll (theme::panel.darker (0.3f)); }

    void BottomBar::resized()
    {
        auto r = getLocalBounds().reduced (10, 6);
        auto take = [&r] (int w) { auto c = r.removeFromLeft (w); r.removeFromLeft (10); return c; };
        mode.setBounds (take (140));
        range.setBounds (take (90));
        bypass.setBounds (r.removeFromRight (80));
        invert.setBounds (r.removeFromRight (80));
        autoGain.setBounds (r.removeFromRight (95));
        outL.setBounds (take (50)); outGain.setBounds (take (110));
        scaleL.setBounds (take (55)); gainScale.setBounds (take (90));
    }
}
