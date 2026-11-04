#include "UI/ResponseDisplay.h"
#include "UI/Strings_fr.h"
#include "UI/Theme.h"
#include "Plugin/PluginProcessor.h"
#include "DSP/BiquadDesign.h"

namespace prisma::ui
{
    namespace
    {
        constexpr double kMinHz = 10.0, kMaxHz = 30000.0;
        constexpr float kPointRadius = 8.0f;
    }

    ResponseDisplay::ResponseDisplay (PrismaProcessor& p) : proc (p), state (p.getState())
    {
        for (auto* prm : p.getParameters())
            if (auto* r = dynamic_cast<juce::RangedAudioParameter*> (prm))
                state.addParameterListener (r->getParameterID(), this);
        startTimerHz (30);
    }

    ResponseDisplay::~ResponseDisplay()
    {
        for (auto* prm : proc.getParameters())
            if (auto* r = dynamic_cast<juce::RangedAudioParameter*> (prm))
                state.removeParameterListener (r->getParameterID(), this);
    }

    void ResponseDisplay::timerCallback()
    {
        if (dirty.exchange (false)) repaint();
    }

    void ResponseDisplay::selectBand (int b)
    {
        if (b == selected) return;
        selected = b;
        repaint();
        if (onSelectionChanged) onSelectionChanged (b);
    }

    juce::RangedAudioParameter* ResponseDisplay::param (int band, const char* name) const
    {
        return state.getParameter (params::bandId (band, name));
    }

    float ResponseDisplay::value (int band, const char* name) const
    {
        return state.getRawParameterValue (params::bandId (band, name))->load();
    }

    void ResponseDisplay::setValue (int band, const char* name, float v)
    {
        if (auto* p = param (band, name))
            p->setValueNotifyingHost (p->convertTo0to1 (v));
    }

    float ResponseDisplay::xForFreq (double hz) const
    {
        return (float) (std::log (hz / kMinHz) / std::log (kMaxHz / kMinHz)) * (float) getWidth();
    }

    double ResponseDisplay::freqForX (float x) const
    {
        return kMinHz * std::pow (kMaxHz / kMinHz, (double) x / std::max (1, getWidth()));
    }

    float ResponseDisplay::yForDb (double db) const
    {
        return (float) ((0.5 - 0.5 * db / (rangeDb * 1.1)) * getHeight());
    }

    double ResponseDisplay::dbForY (float y) const
    {
        return (0.5 - (double) y / std::max (1, getHeight())) * 2.0 * rangeDb * 1.1;
    }

    juce::Point<float> ResponseDisplay::bandPoint (int b) const
    {
        const auto s = proc.readBand (b);
        const double g = dsp::shapeHasGain (s.params.shape) ? s.params.gainDb : 0.0;
        return { xForFreq (s.params.freq), yForDb (juce::jlimit ((double) -rangeDb * 1.05, (double) rangeDb * 1.05, g)) };
    }

    int ResponseDisplay::hitBand (juce::Point<float> pos) const
    {
        int best = -1;
        float bestDist = kPointRadius * 1.6f;
        for (int b = 0; b < params::kNumBands; ++b)
        {
            if (value (b, params::id::used) < 0.5f) continue;
            const float d = bandPoint (b).getDistanceFrom (pos);
            if (d < bestDist) { bestDist = d; best = b; }
        }
        return best;
    }

    void ResponseDisplay::paint (juce::Graphics& g)
    {
        const float w = (float) getWidth(), h = (float) getHeight();
        g.fillAll (theme::background);

        // Grille
        g.setFont (juce::FontOptions (11.0f));
        for (double f : { 20.0, 50.0, 100.0, 200.0, 500.0, 1000.0, 2000.0, 5000.0, 10000.0, 20000.0 })
        {
            const float x = xForFreq (f);
            g.setColour (theme::grid);
            g.drawVerticalLine ((int) x, 0.0f, h);
            g.setColour (theme::textDim);
            g.drawText (f >= 1000.0 ? juce::String ((int) (f / 1000)) + "k" : juce::String ((int) f),
                        juce::Rectangle<float> (x + 3, h - 16, 40, 14), juce::Justification::left);
        }
        const float step = rangeDb <= 3.0f ? 1.0f : rangeDb <= 6.0f ? 2.0f : rangeDb <= 12.0f ? 3.0f : 6.0f;
        for (float db = -rangeDb; db <= rangeDb + 0.01f; db += step)
        {
            const float y = yForDb (db);
            g.setColour (std::abs (db) < 0.01f ? theme::gridStrong : theme::grid);
            g.drawHorizontalLine ((int) y, 0.0f, w);
            g.setColour (theme::textDim);
            g.drawText ((db > 0 ? "+" : "") + juce::String (db, 0), juce::Rectangle<float> (w - 34, y - 13, 30, 12),
                        juce::Justification::right);
        }

        // Réponse numérique exacte : produit des biquads de chaque bande active.
        const double fs = proc.getSampleRate() > 0 ? proc.getSampleRate() : 48000.0;
        const double scale = proc.getGainScale();
        std::array<dsp::DigitalDesign, params::kNumBands> designs;
        std::array<bool, params::kNumBands> active {};
        for (int b = 0; b < params::kNumBands; ++b)
        {
            const auto s = proc.readBand (b);
            active[(size_t) b] = s.used && s.enabled;
            if (active[(size_t) b]) designs[(size_t) b] = dsp::designBand (s.params, fs, scale);
        }

        const int n = std::max (2, getWidth() / 2);
        juce::Path total, sel;
        for (int i = 0; i < n; ++i)
        {
            const float x = w * (float) i / (float) (n - 1);
            const double f = freqForX (x);
            if (f >= 0.5 * fs) break; // la réponse numérique s'arrête à Nyquist
            double db = 0.0, selDb = 0.0;
            for (int b = 0; b < params::kNumBands; ++b)
                    if (active[(size_t) b])
                    {
                        const double d = designs[(size_t) b].magnitudeDb (f, fs);
                        db += d;
                        if (b == selected) selDb = d;
                    }
            const float y = juce::jlimit (-10.0f, h + 10.0f, yForDb (db));
            const float ys = juce::jlimit (-10.0f, h + 10.0f, yForDb (selDb));
            if (i == 0) { total.startNewSubPath (x, y); sel.startNewSubPath (x, ys); }
            else        { total.lineTo (x, y); sel.lineTo (x, ys); }
        }
        if (selected >= 0 && active[(size_t) selected])
        {
            g.setColour (theme::bandColour (selected).withAlpha (0.55f));
            g.strokePath (sel, juce::PathStrokeType (1.5f));
        }
        g.setColour (theme::curve);
        g.strokePath (total, juce::PathStrokeType (2.2f, juce::PathStrokeType::curved));

        // Points de bande
        for (int b = 0; b < params::kNumBands; ++b)
        {
            if (value (b, params::id::used) < 0.5f) continue;
            const auto pt = bandPoint (b);
            auto c = theme::bandColour (b);
            if (value (b, params::id::enabled) < 0.5f) c = c.withSaturation (0.0f).withAlpha (0.5f);
            const auto r = juce::Rectangle<float> (kPointRadius * 2, kPointRadius * 2).withCentre (pt);
            g.setColour (c.withAlpha (b == selected ? 1.0f : 0.8f));
            g.fillEllipse (r);
            if (b == selected)
            {
                g.setColour (theme::text);
                g.drawEllipse (r.expanded (2.0f), 1.5f);
            }
            g.setColour (theme::background);
            g.setFont (juce::FontOptions (10.0f, juce::Font::bold));
            g.drawText (juce::String (b + 1), r, juce::Justification::centred);
        }
    }

    void ResponseDisplay::mouseDown (const juce::MouseEvent& e)
    {
        const int b = hitBand (e.position);
        selectBand (b);
        if (b < 0) return;
        if (e.mods.isPopupMenu()) { showBandMenu (b); return; }
        dragging = b;
        param (b, params::id::freq)->beginChangeGesture();
        param (b, params::id::gain)->beginChangeGesture();
    }

    void ResponseDisplay::mouseDrag (const juce::MouseEvent& e)
    {
        if (dragging < 0) return;
        setValue (dragging, params::id::freq, (float) juce::jlimit ((double) params::kMinFreq, (double) params::kMaxFreq, freqForX (e.position.x)));
        if (dsp::shapeHasGain ((dsp::Shape) (int) value (dragging, params::id::shape)))
            setValue (dragging, params::id::gain, (float) juce::jlimit (-30.0, 30.0, dbForY (e.position.y)));
    }

    void ResponseDisplay::mouseUp (const juce::MouseEvent&)
    {
        if (dragging < 0) return;
        param (dragging, params::id::freq)->endChangeGesture();
        param (dragging, params::id::gain)->endChangeGesture();
        dragging = -1;
    }

    void ResponseDisplay::mouseDoubleClick (const juce::MouseEvent& e)
    {
        if (hitBand (e.position) >= 0) return;
        for (int b = 0; b < params::kNumBands; ++b)
        {
            if (value (b, params::id::used) > 0.5f) continue;
            setValue (b, params::id::shape, (float) (int) dsp::Shape::Bell);
            setValue (b, params::id::placement, 0.0f);
            setValue (b, params::id::q, 1.0f);
            setValue (b, params::id::freq, (float) juce::jlimit ((double) params::kMinFreq, (double) params::kMaxFreq, freqForX (e.position.x)));
            setValue (b, params::id::gain, (float) juce::jlimit (-30.0, 30.0, dbForY (e.position.y)));
            setValue (b, params::id::enabled, 1.0f);
            setValue (b, params::id::used, 1.0f);
            selectBand (b);
            return;
        }
    }

    void ResponseDisplay::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel)
    {
        int b = hitBand (e.position);
        if (b < 0) b = selected;
        if (b < 0 || value (b, params::id::used) < 0.5f) return;
        const auto shape = (dsp::Shape) (int) value (b, params::id::shape);
        const float delta = wheel.deltaY * (wheel.isReversed ? -1.0f : 1.0f);
        if (shape == dsp::Shape::LowCut || shape == dsp::Shape::HighCut)
            setValue (b, params::id::slope, juce::jlimit (6.0f, params::kMaxSlope, value (b, params::id::slope) + (delta > 0 ? 6.0f : -6.0f)));
        else
            setValue (b, params::id::q, juce::jlimit (params::kMinQ, params::kMaxQ, value (b, params::id::q) * std::pow (2.0f, delta * 1.5f)));
    }

    void ResponseDisplay::showBandMenu (int b)
    {
        juce::PopupMenu shapes, slopes, places, m;
        const int curShape = (int) value (b, params::id::shape);
        for (int i = 0; i < params::shapeNames().size(); ++i)
            shapes.addItem (100 + i, params::shapeNames()[i], true, i == curShape);
        const float curSlope = value (b, params::id::slope);
        for (int s : { 6, 12, 18, 24, 30, 36, 48, 72, 96 })
            slopes.addItem (200 + s, juce::String (s) + " dB/oct", true, std::abs (curSlope - (float) s) < 0.01f);
        const int curPlace = (int) value (b, params::id::placement);
        for (int i = 0; i < params::placementNames().size(); ++i)
            places.addItem (400 + i, params::placementNames()[i], true, i == curPlace);

        m.addSectionHeader (str::band + " " + juce::String (b + 1));
        m.addSubMenu (str::shape, shapes);
        m.addSubMenu (str::slope, slopes);
        m.addSubMenu (str::placement, places);
        m.addItem (2, str::active, true, value (b, params::id::enabled) > 0.5f);
        m.addSeparator();
        m.addItem (1, str::remove);

        juce::Component::SafePointer<ResponseDisplay> safe (this);
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this).withMousePosition(), [safe, b] (int r)
        {
            if (safe == nullptr || r == 0) return;
            auto& d = *safe;
            if (r == 1)        { d.setValue (b, params::id::used, 0.0f); d.selectBand (-1); }
            else if (r == 2)   d.setValue (b, params::id::enabled, d.value (b, params::id::enabled) > 0.5f ? 0.0f : 1.0f);
            else if (r < 200)  d.setValue (b, params::id::shape, (float) (r - 100));
            else if (r < 400)  d.setValue (b, params::id::slope, (float) (r - 200));
            else               d.setValue (b, params::id::placement, (float) (r - 400));
        });
    }
}
