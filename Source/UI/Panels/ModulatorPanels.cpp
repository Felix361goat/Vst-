#include "UI/Panels/ModulatorPanels.h"

#include "Params/ParameterIDs.h"
#include "UI/Panels/SynthPanels.h"

namespace nog::ui
{
    namespace
    {
        constexpr int knobRowHeight  = 62;
        constexpr int displayHeight  = 66;
        constexpr int refreshHz      = 10;

        float readParameter (juce::AudioProcessorValueTreeState& state, const juce::String& id)
        {
            if (auto* value = state.getRawParameterValue (id))
                return value->load();

            return 0.0f;
        }

        /** Maps a stage's length onto the display, log-scaled so that a 2 ms
            attack next to a 10 s release is still visible. */
        float stageWidth (float milliseconds)
        {
            return std::log10 (1.0f + juce::jmax (0.0f, milliseconds));
        }

        /** The curve shaping used by dsp::Envelope, duplicated here so the
            drawing matches what is heard. */
        float shapeCurve (float x, float curve)
        {
            if (std::abs (curve) < 1.0e-4f)
                return x;

            return std::pow (juce::jlimit (0.0f, 1.0f, x), std::pow (4.0f, -curve));
        }
    }

    // -----------------------------------------------------------------------
    EnvelopeDisplay::EnvelopeDisplay (juce::AudioProcessorValueTreeState& stateToUse, int envelopeIndex)
        : state (stateToUse), index (envelopeIndex)
    {
        startTimerHz (refreshHz);
    }

    void EnvelopeDisplay::timerCallback()
    {
        const std::array<float, 8> current {
            readParameter (state, ids::env (index, ids::envAttack)),
            readParameter (state, ids::env (index, ids::envHold)),
            readParameter (state, ids::env (index, ids::envDecay)),
            readParameter (state, ids::env (index, ids::envSustain)),
            readParameter (state, ids::env (index, ids::envRelease)),
            readParameter (state, ids::env (index, ids::envAttackCurve)),
            readParameter (state, ids::env (index, ids::envDecayCurve)),
            readParameter (state, ids::env (index, ids::envRelCurve))
        };

        if (current == lastValues)
            return;

        lastValues = current;
        repaint();
    }

    void EnvelopeDisplay::paint (juce::Graphics& g)
    {
        const auto bounds = getLocalBounds().toFloat().reduced (4.0f);

        g.setColour (colours::background);
        g.fillRoundedRectangle (bounds, 3.0f);
        g.setColour (colours::border);
        g.drawRoundedRectangle (bounds.reduced (0.5f), 3.0f, 1.0f);

        const auto attack   = lastValues[0];
        const auto hold     = lastValues[1];
        const auto decay    = lastValues[2];
        const auto sustain  = juce::jlimit (0.0f, 1.0f, lastValues[3]);
        const auto release  = lastValues[4];
        const auto atkCurve = lastValues[5];
        const auto decCurve = lastValues[6];
        const auto relCurve = lastValues[7];

        // A fixed sustain segment keeps the shape readable; the real sustain
        // lasts as long as the key is held.
        constexpr float sustainSegment = 0.6f;

        const auto widths = std::array {
            stageWidth (attack), stageWidth (hold), stageWidth (decay),
            sustainSegment, stageWidth (release)
        };

        auto total = 0.0f;
        for (auto w : widths)
            total += w;

        if (total <= 0.0f)
            return;

        const auto plot   = bounds.reduced (6.0f);
        const auto scaleX = plot.getWidth() / total;

        const auto yFor = [&plot] (float level)
        {
            return plot.getBottom() - juce::jlimit (0.0f, 1.0f, level) * plot.getHeight();
        };

        juce::Path path;
        path.startNewSubPath (plot.getX(), yFor (0.0f));

        auto x = plot.getX();

        // Each curved stage is drawn as a short polyline; twenty steps is more
        // than enough at this size and keeps the path cheap to build.
        constexpr int steps = 20;

        const auto addStage = [&] (float widthInPixels, float from, float to, float curve)
        {
            if (widthInPixels <= 0.0f)
                return;

            for (int i = 1; i <= steps; ++i)
            {
                const auto t      = static_cast<float> (i) / static_cast<float> (steps);
                const auto shaped = shapeCurve (t, curve);

                path.lineTo (x + widthInPixels * t, yFor (from + (to - from) * shaped));
            }

            x += widthInPixels;
        };

        addStage (widths[0] * scaleX, 0.0f, 1.0f, atkCurve);

        if (widths[1] > 0.0f)
        {
            x += widths[1] * scaleX;
            path.lineTo (x, yFor (1.0f));
        }

        addStage (widths[2] * scaleX, 1.0f, sustain, decCurve);

        x += widths[3] * scaleX;
        path.lineTo (x, yFor (sustain));

        addStage (widths[4] * scaleX, sustain, 0.0f, relCurve);

        g.setColour (colours::accent);
        g.strokePath (path, juce::PathStrokeType (1.6f, juce::PathStrokeType::curved));

        // A soft fill under the curve makes the shape easier to read at a glance.
        auto filled = path;
        filled.lineTo (x, plot.getBottom());
        filled.lineTo (plot.getX(), plot.getBottom());
        filled.closeSubPath();

        g.setColour (colours::accent.withAlpha (0.12f));
        g.fillPath (filled);
    }

    // -----------------------------------------------------------------------
    EnvelopePanel::EnvelopePanel (juce::AudioProcessorValueTreeState& state, int index)
        : display      (state, index),
          attack       (state, ids::env (index, ids::envAttack), "Attack"),
          hold         (state, ids::env (index, ids::envHold), "Hold"),
          decay        (state, ids::env (index, ids::envDecay), "Decay"),
          sustain      (state, ids::env (index, ids::envSustain), "Sustain"),
          release      (state, ids::env (index, ids::envRelease), "Release"),
          attackCurve  (state, ids::env (index, ids::envAttackCurve), "Atk Crv"),
          decayCurve   (state, ids::env (index, ids::envDecayCurve), "Dec Crv"),
          releaseCurve (state, ids::env (index, ids::envRelCurve), "Rel Crv")
    {
        addAllChildren (*this, { &display,
                                 &attack, &hold, &decay, &sustain, &release,
                                 &attackCurve, &decayCurve, &releaseCurve });
    }

    void EnvelopePanel::resized()
    {
        auto bounds = getLocalBounds().reduced (4);

        display.setBounds (bounds.removeFromTop (displayHeight));
        bounds.removeFromTop (4);

        layoutRow (bounds.removeFromTop (knobRowHeight),
                   { &attack, &hold, &decay, &sustain, &release });
        layoutRow (bounds.removeFromTop (knobRowHeight),
                   { &attackCurve, &decayCurve, &releaseCurve });
    }

    // -----------------------------------------------------------------------
    LfoDisplay::LfoDisplay (juce::AudioProcessorValueTreeState& stateToUse, int lfoIndex)
        : state (stateToUse), index (lfoIndex)
    {
        startTimerHz (refreshHz);
    }

    void LfoDisplay::timerCallback()
    {
        const auto shape = static_cast<int> (readParameter (state, ids::lfo (index, ids::lfoShape)));
        const auto phase = readParameter (state, ids::lfo (index, ids::lfoPhase));

        if (shape == lastShape && juce::approximatelyEqual (phase, lastPhase))
            return;

        lastShape = shape;
        lastPhase = phase;
        repaint();
    }

    void LfoDisplay::paint (juce::Graphics& g)
    {
        const auto bounds = getLocalBounds().toFloat().reduced (4.0f);

        g.setColour (colours::background);
        g.fillRoundedRectangle (bounds, 3.0f);
        g.setColour (colours::border);
        g.drawRoundedRectangle (bounds.reduced (0.5f), 3.0f, 1.0f);

        const auto plot = bounds.reduced (6.0f);

        g.setColour (colours::knobTrack);
        g.drawHorizontalLine (juce::roundToInt (plot.getCentreY()), plot.getX(), plot.getRight());

        // The shape is generated by the same LFO class the audio thread uses, so
        // the picture cannot drift from the sound. Running it at a sample rate
        // equal to the point count makes one cycle span the display exactly.
        constexpr int points = 200;

        dsp::Lfo lfo;
        lfo.prepare (static_cast<double> (points));
        lfo.setShape (static_cast<dsp::Lfo::Shape> (juce::jlimit (0, 6, lastShape)));
        lfo.setRateHz (1.0f);
        lfo.setBipolar (true);
        lfo.setPhaseOffset (lastPhase);
        lfo.setSmoothing (0.0f);
        lfo.setRiseMs (0.0f);

        juce::Path path;

        for (int i = 0; i < points; ++i)
        {
            const auto value = lfo.getNextValue();
            const auto x = plot.getX() + plot.getWidth() * static_cast<float> (i) / static_cast<float> (points - 1);
            const auto y = plot.getCentreY() - value * plot.getHeight() * 0.45f;

            if (i == 0)
                path.startNewSubPath (x, y);
            else
                path.lineTo (x, y);
        }

        g.setColour (colours::modulation);
        g.strokePath (path, juce::PathStrokeType (1.6f, juce::PathStrokeType::curved));
    }

    // -----------------------------------------------------------------------
    LfoPanel::LfoPanel (juce::AudioProcessorValueTreeState& state, const ModMatrix& matrix, int index)
        : display  (state, index),
          shape    (state, ids::lfo (index, ids::lfoShape), "Shape"),
          syncMode (state, ids::lfo (index, ids::lfoSyncMode), "Sync"),
          division (state, ids::lfo (index, ids::lfoRateSync), "Division"),
          trigger  (state, ids::lfo (index, ids::lfoTrigger), "Trigger"),
          bipolar  (state, ids::lfo (index, ids::lfoBipolar), "BIPOLAR"),
          rate     (state, ids::lfo (index, ids::lfoRateHz), "Rate"),
          phase    (state, ids::lfo (index, ids::lfoPhase), "Phase"),
          rise     (state, ids::lfo (index, ids::lfoRise), "Rise"),
          smooth   (state, ids::lfo (index, ids::lfoSmooth), "Smooth")
    {
        addAllChildren (*this, { &display,
                                 &shape, &syncMode, &division, &trigger, &bipolar,
                                 &rate, &phase, &rise, &smooth });

        rate.showModulationFor (matrix, static_cast<mod::Dest> (static_cast<int> (mod::Dest::Lfo1Rate) + index));
    }

    void LfoPanel::resized()
    {
        auto bounds = getLocalBounds().reduced (4);

        display.setBounds (bounds.removeFromTop (displayHeight));
        bounds.removeFromTop (4);

        layoutRow (bounds.removeFromTop (44), { &shape, &syncMode, &division, &trigger }, 4);
        bounds.removeFromTop (4);

        auto knobRow = bounds.removeFromTop (knobRowHeight);
        bipolar.setBounds (knobRow.removeFromRight (74).withSizeKeepingCentre (70, 22));
        layoutRow (knobRow, { &rate, &phase, &rise, &smooth });
    }

    // -----------------------------------------------------------------------
    MacroStrip::MacroStrip (juce::AudioProcessorValueTreeState& state)
    {
        for (int i = 0; i < ids::numMacros; ++i)
        {
            auto knob = std::make_unique<Knob> (state, ids::macro (i), "Macro " + juce::String (i + 1));
            addAndMakeVisible (*knob);
            macros[static_cast<size_t> (i)] = std::move (knob);
        }
    }

    void MacroStrip::resized()
    {
        // Two by two rather than a single row: the macro section is tall and
        // narrow, so a 4-wide row would leave the knobs tiny.
        auto bounds = getLocalBounds().reduced (2);
        const auto rowHeight = juce::jmin (bounds.getHeight() / 2, 90);

        layoutRow (bounds.removeFromTop (rowHeight), { macros[0].get(), macros[1].get() });
        layoutRow (bounds.removeFromTop (rowHeight), { macros[2].get(), macros[3].get() });
    }

    // -----------------------------------------------------------------------
    ModulatorsPanel::ModulatorsPanel (juce::AudioProcessorValueTreeState& state, const ModMatrix& matrix)
    {
        tabs.setTabBarDepth (24);
        tabs.setOutline (0);

        for (int i = 0; i < ids::numEnvelopes; ++i)
        {
            envelopes[static_cast<size_t> (i)] = std::make_unique<EnvelopePanel> (state, i);
            tabs.addTab ("ENV " + juce::String (i + 1), colours::panel,
                         envelopes[static_cast<size_t> (i)].get(), false);
        }

        for (int i = 0; i < ids::numLfos; ++i)
        {
            lfos[static_cast<size_t> (i)] = std::make_unique<LfoPanel> (state, matrix, i);
            tabs.addTab ("LFO " + juce::String (i + 1), colours::panel,
                         lfos[static_cast<size_t> (i)].get(), false);
        }

        addAndMakeVisible (tabs);
    }

    void ModulatorsPanel::resized()
    {
        tabs.setBounds (getLocalBounds());
    }
}
