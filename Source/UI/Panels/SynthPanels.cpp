#include "UI/Panels/SynthPanels.h"

#include "Params/ParameterIDs.h"

namespace nog::ui
{
    namespace
    {
        constexpr int controlRowHeight = 30;
        constexpr int knobRowHeight    = 62;
    }

    void layoutRow (juce::Rectangle<int> area, const std::vector<juce::Component*>& components, int gap)
    {
        if (components.empty() || area.isEmpty())
            return;

        const auto count = static_cast<int> (components.size());
        const auto width = area.getWidth() / count;

        for (int i = 0; i < count; ++i)
        {
            // The last cell takes the remainder so rounding never leaves a gap
            // down the right-hand edge.
            auto cell = i == count - 1 ? area : area.removeFromLeft (width);

            if (components[static_cast<size_t> (i)] != nullptr)
                components[static_cast<size_t> (i)]->setBounds (cell.reduced (gap, 0));
        }
    }

    // -----------------------------------------------------------------------
    OscillatorPanel::OscillatorPanel (juce::AudioProcessorValueTreeState& state,
                                      const ModMatrix& matrix, int index)
        : enable      (state, ids::osc (index, ids::oscEnable), "ON"),
          toFilter    (state, ids::osc (index, ids::oscToFilter), "> FILTER"),
          wave        (state, ids::osc (index, ids::oscWave), {}),
          warpMode    (state, ids::osc (index, ids::oscWarpMode), {}),
          level       (state, ids::osc (index, ids::oscLevel), "Level"),
          pan         (state, ids::osc (index, ids::oscPan), "Pan"),
          wtPos       (state, ids::osc (index, ids::oscWtPos), "WT Pos"),
          warp        (state, ids::osc (index, ids::oscWarpAmount), "Warp"),
          detune      (state, ids::osc (index, ids::oscDetune), "Detune"),
          unison      (state, ids::osc (index, ids::oscUnison), "Unison"),
          blend       (state, ids::osc (index, ids::oscBlend), "Blend"),
          width       (state, ids::osc (index, ids::oscUniWidth), "Width"),
          phase       (state, ids::osc (index, ids::oscPhase), "Phase"),
          phaseRandom (state, ids::osc (index, ids::oscPhaseRand), "Rand"),
          octave      (state, ids::osc (index, ids::oscOctave), "Octave"),
          semi        (state, ids::osc (index, ids::oscSemi), "Semi"),
          fine        (state, ids::osc (index, ids::oscFine), "Fine")
    {
        addAllChildren (*this, { &enable, &toFilter, &wave, &warpMode,
                                 &level, &pan, &wtPos, &warp, &detune,
                                 &unison, &blend, &width, &phase, &phaseRandom,
                                 &octave, &semi, &fine });

        const auto levelDest  = index == 0 ? mod::Dest::Osc1Level  : mod::Dest::Osc2Level;
        const auto panDest    = index == 0 ? mod::Dest::Osc1Pan    : mod::Dest::Osc2Pan;
        const auto wtDest     = index == 0 ? mod::Dest::Osc1WtPos  : mod::Dest::Osc2WtPos;
        const auto warpDest   = index == 0 ? mod::Dest::Osc1Warp   : mod::Dest::Osc2Warp;
        const auto detuneDest = index == 0 ? mod::Dest::Osc1Detune : mod::Dest::Osc2Detune;
        const auto phaseDest  = index == 0 ? mod::Dest::Osc1Phase  : mod::Dest::Osc2Phase;

        level.showModulationFor (matrix, levelDest);
        pan.showModulationFor (matrix, panDest);
        wtPos.showModulationFor (matrix, wtDest);
        warp.showModulationFor (matrix, warpDest);
        detune.showModulationFor (matrix, detuneDest);
        phase.showModulationFor (matrix, phaseDest);
    }

    void OscillatorPanel::resized()
    {
        auto bounds = getLocalBounds();

        auto topRow = bounds.removeFromTop (controlRowHeight);
        enable.setBounds (topRow.removeFromLeft (46).reduced (2, 4));
        toFilter.setBounds (topRow.removeFromRight (74).reduced (2, 4));
        layoutRow (topRow.reduced (0, 4), { &wave, &warpMode });

        layoutRow (bounds.removeFromTop (knobRowHeight), { &level, &pan, &wtPos, &warp, &detune });
        layoutRow (bounds.removeFromTop (knobRowHeight), { &unison, &blend, &width, &phase, &phaseRandom });
        layoutRow (bounds.removeFromTop (knobRowHeight), { &octave, &semi, &fine });
    }

    // -----------------------------------------------------------------------
    SubPanel::SubPanel (juce::AudioProcessorValueTreeState& state, const ModMatrix& matrix)
        : enable   (state, ids::subEnable, "ON"),
          toFilter (state, ids::subToFilter, "> FILTER"),
          wave     (state, ids::subWave, {}),
          level    (state, ids::subLevel, "Level"),
          pan      (state, ids::subPan, "Pan"),
          octave   (state, ids::subOctave, "Octave")
    {
        addAllChildren (*this, { &enable, &toFilter, &wave, &level, &pan, &octave });

        level.showModulationFor (matrix, mod::Dest::SubLevel);
        pan.showModulationFor (matrix, mod::Dest::SubPan);
    }

    void SubPanel::resized()
    {
        auto bounds = getLocalBounds();

        auto topRow = bounds.removeFromTop (controlRowHeight);
        enable.setBounds (topRow.removeFromLeft (46).reduced (2, 4));
        toFilter.setBounds (topRow.removeFromRight (74).reduced (2, 4));
        wave.setBounds (topRow.reduced (2, 4));

        layoutRow (bounds.removeFromTop (knobRowHeight), { &level, &pan, &octave });
    }

    // -----------------------------------------------------------------------
    NoisePanel::NoisePanel (juce::AudioProcessorValueTreeState& state, const ModMatrix& matrix)
        : enable   (state, ids::noiseEnable, "ON"),
          toFilter (state, ids::noiseToFilter, "> FILTER"),
          colour   (state, ids::noiseColour, {}),
          level    (state, ids::noiseLevel, "Level"),
          pan      (state, ids::noisePan, "Pan")
    {
        addAllChildren (*this, { &enable, &toFilter, &colour, &level, &pan });

        level.showModulationFor (matrix, mod::Dest::NoiseLevel);
        pan.showModulationFor (matrix, mod::Dest::NoisePan);
    }

    void NoisePanel::resized()
    {
        auto bounds = getLocalBounds();

        auto topRow = bounds.removeFromTop (controlRowHeight);
        enable.setBounds (topRow.removeFromLeft (46).reduced (2, 4));
        toFilter.setBounds (topRow.removeFromRight (74).reduced (2, 4));
        colour.setBounds (topRow.reduced (2, 4));

        layoutRow (bounds.removeFromTop (knobRowHeight), { &level, &pan });
    }

    // -----------------------------------------------------------------------
    FilterPanel::FilterPanel (juce::AudioProcessorValueTreeState& state, const ModMatrix& matrix)
        : enable    (state, ids::filterEnable, "ON"),
          type      (state, ids::filterType, {}),
          cutoff    (state, ids::filterCutoff, "Cutoff"),
          resonance (state, ids::filterReso, "Reso"),
          drive     (state, ids::filterDrive, "Drive"),
          mix       (state, ids::filterMix, "Mix"),
          keytrack  (state, ids::filterKeytrack, "Key Trk")
    {
        addAllChildren (*this, { &enable, &type, &cutoff, &resonance, &drive, &mix, &keytrack });

        cutoff.showModulationFor (matrix, mod::Dest::FilterCutoff);
        resonance.showModulationFor (matrix, mod::Dest::FilterReso);
        drive.showModulationFor (matrix, mod::Dest::FilterDrive);
        mix.showModulationFor (matrix, mod::Dest::FilterMix);
    }

    void FilterPanel::resized()
    {
        auto bounds = getLocalBounds();

        auto topRow = bounds.removeFromTop (controlRowHeight);
        enable.setBounds (topRow.removeFromLeft (46).reduced (2, 4));
        type.setBounds (topRow.reduced (2, 4));

        layoutRow (bounds.removeFromTop (knobRowHeight), { &cutoff, &resonance, &drive, &mix, &keytrack });
    }

    // -----------------------------------------------------------------------
    GlobalPanel::GlobalPanel (juce::AudioProcessorValueTreeState& state)
        : voiceMode    (state, ids::voiceMode, "Voice Mode"),
          glideMode    (state, ids::glideMode, "Glide Mode"),
          oversampling (state, ids::oversampling, "Oversampling"),
          polyphony    (state, ids::polyphony, "Voices"),
          glideTime    (state, ids::glideTime, "Glide"),
          bendRange    (state, ids::pitchBendRange, "Bend Range"),
          velocitySens (state, ids::velocitySens, "Vel Sens")
    {
        addAllChildren (*this, { &voiceMode, &glideMode, &oversampling,
                                 &polyphony, &glideTime, &bendRange, &velocitySens });

        help.setText ("Oversampling is reserved for the DSP work still to come and currently has no effect.",
                      juce::dontSendNotification);
        help.setFont (juce::Font (juce::FontOptions (11.0f)));
        help.setColour (juce::Label::textColourId, colours::dimText);
        help.setJustificationType (juce::Justification::topLeft);
        addAndMakeVisible (help);
    }

    void GlobalPanel::resized()
    {
        auto bounds = getLocalBounds();

        layoutRow (bounds.removeFromTop (44), { &voiceMode, &glideMode, &oversampling }, 6);
        bounds.removeFromTop (8);
        layoutRow (bounds.removeFromTop (knobRowHeight), { &polyphony, &glideTime, &bendRange, &velocitySens });
        bounds.removeFromTop (10);

        help.setBounds (bounds.removeFromTop (34).reduced (4, 0));
    }
}
