#include "UI/Panels/SynthPanels.h"

#include "Params/ParameterIDs.h"
#include "DSP/SampleBank.h"
#include "PluginProcessor.h"

namespace nog::ui
{
    namespace
    {
        constexpr int controlRowHeight = 30;
        constexpr int knobRowHeight    = 76;
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
    OscillatorPanel::OscillatorPanel (NogSuiteProcessor& processorToUse,
                                      const ModMatrix& matrix, int index)
        : processor       (processorToUse),
          oscillatorIndex (index),
          enable      (processorToUse.getValueTreeState(), ids::osc (index, ids::oscEnable), "ON"),
          toFilter    (processorToUse.getValueTreeState(), ids::osc (index, ids::oscToFilter), "> FILTER"),
          mode        (processorToUse.getValueTreeState(), ids::osc (index, ids::oscMode), {}),
          wave        (processorToUse.getValueTreeState(), ids::osc (index, ids::oscWave), {}),
          warpMode    (processorToUse.getValueTreeState(), ids::osc (index, ids::oscWarpMode), {}),
          level       (processorToUse.getValueTreeState(), ids::osc (index, ids::oscLevel), "Level"),
          pan         (processorToUse.getValueTreeState(), ids::osc (index, ids::oscPan), "Pan"),
          wtPos       (processorToUse.getValueTreeState(), ids::osc (index, ids::oscWtPos), "WT Pos"),
          warp        (processorToUse.getValueTreeState(), ids::osc (index, ids::oscWarpAmount), "Warp"),
          detune      (processorToUse.getValueTreeState(), ids::osc (index, ids::oscDetune), "Detune"),
          unison      (processorToUse.getValueTreeState(), ids::osc (index, ids::oscUnison), "Unison"),
          blend       (processorToUse.getValueTreeState(), ids::osc (index, ids::oscBlend), "Blend"),
          width       (processorToUse.getValueTreeState(), ids::osc (index, ids::oscUniWidth), "Width"),
          phase       (processorToUse.getValueTreeState(), ids::osc (index, ids::oscPhase), "Phase"),
          phaseRandom (processorToUse.getValueTreeState(), ids::osc (index, ids::oscPhaseRand), "Rand"),
          octave      (processorToUse.getValueTreeState(), ids::osc (index, ids::oscOctave), "Octave"),
          semi        (processorToUse.getValueTreeState(), ids::osc (index, ids::oscSemi), "Semi"),
          fine        (processorToUse.getValueTreeState(), ids::osc (index, ids::oscFine), "Fine")
    {
        addAllChildren (*this, { &enable, &toFilter, &mode, &wave, &warpMode, &sampleButton,
                                 &level, &pan, &wtPos, &warp, &detune,
                                 &unison, &blend, &width, &phase, &phaseRandom,
                                 &octave, &semi, &fine });

        sampleButton.onClick = [this] { showSampleMenu(); };
        sampleButton.setTooltip ("Load an audio file for this oscillator to play");

        updateModeVisibility();
        startTimerHz (6);

        const auto levelDest  = index == 0 ? mod::Dest::Osc1Level  : mod::Dest::Osc2Level;
        const auto panDest    = index == 0 ? mod::Dest::Osc1Pan    : mod::Dest::Osc2Pan;
        const auto wtDest     = index == 0 ? mod::Dest::Osc1WtPos  : mod::Dest::Osc2WtPos;
        const auto warpDest   = index == 0 ? mod::Dest::Osc1Warp   : mod::Dest::Osc2Warp;
        const auto detuneDest = index == 0 ? mod::Dest::Osc1Detune : mod::Dest::Osc2Detune;
        const auto phaseDest  = index == 0 ? mod::Dest::Osc1Phase  : mod::Dest::Osc2Phase;

        // Osc 1 and Osc 2 get their own colours so a glance tells you which
        // half of the page you are working on.
        const auto capColour = index == 0 ? colours::candyRed : colours::candyBlue;

        for (auto* knob : { &level, &pan, &wtPos, &warp, &detune,
                            &unison, &blend, &width, &phase, &phaseRandom,
                            &octave, &semi, &fine })
            knob->setAccentColour (capColour);

        level.showModulationFor (matrix, levelDest);
        pan.showModulationFor (matrix, panDest);
        wtPos.showModulationFor (matrix, wtDest);
        warp.showModulationFor (matrix, warpDest);
        detune.showModulationFor (matrix, detuneDest);
        phase.showModulationFor (matrix, phaseDest);
    }

    void OscillatorPanel::timerCallback()
    {
        const auto currentMode = mode.box.getSelectedItemIndex();
        const auto name = processor.getSampleName (oscillatorIndex);

        if (currentMode == lastMode && name == lastSampleName)
            return;

        lastMode = currentMode;
        lastSampleName = name;

        // The button doubles as the readout: there is no room in the header for
        // a separate label, and the file name is the only thing worth showing.
        sampleButton.setButtonText (name.isNotEmpty() ? name.toUpperCase() : "LOAD SAMPLE");

        updateModeVisibility();
    }

    void OscillatorPanel::updateModeVisibility()
    {
        const auto sampleMode = mode.box.getSelectedItemIndex() == 1;

        wave.setVisible (! sampleMode);
        sampleButton.setVisible (sampleMode);

        // In sample mode the morph control becomes the playback start offset,
        // so it is relabelled rather than hidden.
        wtPos.setLabelText (sampleMode ? "Start" : "WT Pos");
    }

    void OscillatorPanel::showSampleMenu()
    {
        const auto loaded = processor.getSampleName (oscillatorIndex).isNotEmpty();
        auto& state = processor.getValueTreeState();
        const auto index = oscillatorIndex;

        juce::PopupMenu menu;
        menu.addSectionHeader ("Sample");
        menu.addItem ("Load audio file...", [this] { promptForSample(); });

        // The generated character samples: things a wavetable cannot be,
        // because they are transients or evolve over their own length.
        juce::PopupMenu builtIns;
        const auto builtInNames = dsp::SampleBank::getNames();

        for (int i = 0; i < builtInNames.size(); ++i)
            builtIns.addItem (builtInNames[i], [this, i] { processor.loadBuiltInSample (oscillatorIndex, i); });

        menu.addSubMenu ("Built-in", builtIns);

        menu.addItem (juce::PopupMenu::Item ("Clear")
                          .setEnabled (loaded)
                          .setAction ([this] { processor.clearSampleForOscillator (oscillatorIndex); }));
        menu.addSeparator();

        // Playback options live here rather than on the panel, where there is
        // no room for controls that only apply to one of the two modes.
        if (auto* loop = dynamic_cast<juce::AudioParameterChoice*> (
                state.getParameter (ids::osc (index, ids::oscSampleLoop))))
        {
            juce::PopupMenu loopMenu;

            for (int i = 0; i < loop->choices.size(); ++i)
                loopMenu.addItem (juce::PopupMenu::Item (loop->choices[i])
                                      .setTicked (loop->getIndex() == i)
                                      .setAction ([loop, i]
                                      {
                                          loop->beginChangeGesture();
                                          loop->setValueNotifyingHost (loop->convertTo0to1 (static_cast<float> (i)));
                                          loop->endChangeGesture();
                                      }));

            menu.addSubMenu ("Playback", loopMenu);
        }

        if (auto* root = dynamic_cast<juce::AudioParameterInt*> (
                state.getParameter (ids::osc (index, ids::oscSampleRoot))))
        {
            juce::PopupMenu rootMenu;

            // One entry per octave of C is enough: the note the file plays back
            // untransposed is nearly always a round number.
            for (int note = 24; note <= 96; note += 12)
                rootMenu.addItem (juce::PopupMenu::Item (juce::MidiMessage::getMidiNoteName (note, true, true, 4))
                                      .setTicked (root->get() == note)
                                      .setAction ([root, note]
                                      {
                                          root->beginChangeGesture();
                                          root->setValueNotifyingHost (root->convertTo0to1 (static_cast<float> (note)));
                                          root->endChangeGesture();
                                      }));

            menu.addSubMenu ("Root note", rootMenu);
        }

        menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&sampleButton));
    }

    void OscillatorPanel::promptForSample()
    {
        fileChooser = std::make_unique<juce::FileChooser> (
            "Choose an audio file",
            juce::File::getSpecialLocation (juce::File::userMusicDirectory),
            "*.wav;*.aiff;*.aif;*.flac;*.ogg;*.mp3");

        const auto browserFlags = juce::FileBrowserComponent::openMode
                                | juce::FileBrowserComponent::canSelectFiles;

        fileChooser->launchAsync (browserFlags, [this] (const juce::FileChooser& chooser)
        {
            const auto file = chooser.getResult();

            if (file == juce::File())
                return;

            if (! processor.loadSampleForOscillator (oscillatorIndex, file))
                juce::NativeMessageBox::showMessageBoxAsync (
                    juce::MessageBoxIconType::WarningIcon,
                    "Could not load that file",
                    file.getFileName() + " could not be read as audio, or contains only silence. "
                    "WAV, AIFF, FLAC and Ogg are supported.");
        });
    }

    void OscillatorPanel::resized()
    {
        auto bounds = getLocalBounds();

        auto topRow = bounds.removeFromTop (controlRowHeight);
        enable.setBounds (topRow.removeFromLeft (46).reduced (2, 4));
        toFilter.setBounds (topRow.removeFromRight (74).reduced (2, 4));

        // Mode picker, then whichever source selector that mode calls for.
        mode.setBounds (topRow.removeFromLeft (86).reduced (2, 4));

        auto sourceArea = topRow.removeFromLeft (topRow.getWidth() / 2);
        wave.setBounds (sourceArea.reduced (2, 4));
        sampleButton.setBounds (sourceArea.reduced (2, 5));

        warpMode.setBounds (topRow.reduced (2, 4));

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

        for (auto* knob : { &level, &pan, &octave })
            knob->setAccentColour (colours::candyPurple);

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

        for (auto* knob : { &level, &pan })
            knob->setAccentColour (colours::candyOrange);

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

        for (auto* knob : { &cutoff, &resonance, &drive, &mix, &keytrack })
            knob->setAccentColour (colours::candyGreen);

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

        for (auto* knob : { &polyphony, &glideTime, &bendRange, &velocitySens })
            knob->setAccentColour (colours::candyGreen);

        help.setText ("Oversampling runs the voices at a higher rate before downsampling, "
                      "which removes the aliasing the filter drive would otherwise produce. "
                      "It costs CPU in proportion.",
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

    ArpPanel::ArpPanel (juce::AudioProcessorValueTreeState& state)
        : enable  (state, ids::arpEnable, "Arp On"),
          mode    (state, ids::arpMode, "Mode"),
          rate    (state, ids::arpRate, "Rate"),
          octaves (state, ids::arpOctaves, "Octaves"),
          gate    (state, ids::arpGate, "Gate"),
          swing   (state, ids::arpSwing, "Swing")
    {
        addAllChildren (*this, { &enable, &mode, &rate, &octaves, &gate, &swing });

        for (auto* knob : { &octaves, &gate, &swing })
            knob->setAccentColour (colours::candyPurple);

        help.setText ("Holds the keys you press and plays them back one at a time, locked to "
                      "the host tempo. Gate sets how long each step rings; swing pushes every "
                      "second step late.",
                      juce::dontSendNotification);
        help.setFont (juce::Font (juce::FontOptions (11.0f)));
        help.setColour (juce::Label::textColourId, colours::dimText);
        help.setJustificationType (juce::Justification::topLeft);
        addAndMakeVisible (help);
    }

    void ArpPanel::resized()
    {
        auto bounds = getLocalBounds();

        auto top = bounds.removeFromTop (44);
        enable.setBounds (top.removeFromLeft (90));
        top.removeFromLeft (6);
        layoutRow (top, { &mode, &rate }, 6);

        bounds.removeFromTop (8);
        layoutRow (bounds.removeFromTop (knobRowHeight), { &octaves, &gate, &swing });
        bounds.removeFromTop (10);

        help.setBounds (bounds.removeFromTop (34).reduced (4, 0));
    }
}
