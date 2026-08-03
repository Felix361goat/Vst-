#pragma once

#include <juce_core/juce_core.h>

/**
    Central registry of parameter identifiers.

    Every host-visible parameter is addressed by a string ID built here and
    nowhere else. Keeping the ID construction in one place means a rename is a
    single edit, and it makes the "did I spell it the same in the editor?" class
    of bug impossible.

    IDs are stable API: once a plugin has shipped, changing an ID orphans that
    parameter in every project a user has saved. Add new ones, do not rename old
    ones. If a parameter genuinely has to change meaning, give it a new ID and
    migrate the old value in PresetManager.
*/
namespace nog::ids
{
    /** Bumped only when the parameter *layout* changes in a way hosts must be
        told about. JUCE folds this into the VST3 parameter hash.            */
    inline constexpr int parameterVersion = 1;

    // -- module counts ------------------------------------------------------
    inline constexpr int numOscillators = 2;
    inline constexpr int numEnvelopes   = 3;
    inline constexpr int numLfos        = 4;
    inline constexpr int numMacros      = 4;
    inline constexpr int numMatrixSlots = 16;
    inline constexpr int numFxSlots     = 6;

    // -- helpers ------------------------------------------------------------
    /** Builds "osc1_level" style IDs. @p index is zero-based, IDs are one-based. */
    inline juce::String indexed (juce::StringRef prefix, int index, juce::StringRef suffix)
    {
        return juce::String (prefix) + juce::String (index + 1) + "_" + juce::String (suffix);
    }

    inline juce::String osc    (int i, juce::StringRef p) { return indexed ("osc", i, p); }
    inline juce::String env    (int i, juce::StringRef p) { return indexed ("env", i, p); }
    inline juce::String lfo    (int i, juce::StringRef p) { return indexed ("lfo", i, p); }
    inline juce::String matrix (int i, juce::StringRef p) { return indexed ("mod", i, p); }
    inline juce::String fx     (int i, juce::StringRef p) { return indexed ("fx",  i, p); }
    inline juce::String macro  (int i)                    { return "macro" + juce::String (i + 1); }

    // -- global -------------------------------------------------------------
    inline constexpr const char* masterGain     = "master_gain";
    inline constexpr const char* polyphony      = "poly_voices";
    inline constexpr const char* voiceMode      = "poly_mode";       // poly / mono / legato
    inline constexpr const char* glideTime      = "glide_time";
    inline constexpr const char* glideMode      = "glide_mode";      // off / always / legato
    inline constexpr const char* pitchBendRange = "bend_range";
    inline constexpr const char* velocitySens   = "velocity_sens";
    inline constexpr const char* oversampling   = "oversampling";

    // -- arpeggiator --------------------------------------------------------
    inline constexpr const char* arpEnable      = "arp_on";
    inline constexpr const char* arpMode        = "arp_mode";
    inline constexpr const char* arpRate        = "arp_rate";
    inline constexpr const char* arpOctaves     = "arp_octaves";
    inline constexpr const char* arpGate        = "arp_gate";
    inline constexpr const char* arpSwing       = "arp_swing";

    // -- oscillator (per index) --------------------------------------------
    inline constexpr const char* oscEnable      = "enable";
    inline constexpr const char* oscLevel       = "level";
    inline constexpr const char* oscPan         = "pan";
    inline constexpr const char* oscWave        = "wave";            // placeholder for wavetable select
    inline constexpr const char* oscWtPos       = "wtpos";
    inline constexpr const char* oscWarpMode    = "warpmode";
    inline constexpr const char* oscWarpAmount  = "warp";
    inline constexpr const char* oscUnison      = "unison";
    inline constexpr const char* oscDetune      = "detune";
    inline constexpr const char* oscBlend       = "blend";
    inline constexpr const char* oscUniWidth    = "uniwidth";
    inline constexpr const char* oscPhase       = "phase";
    inline constexpr const char* oscPhaseRand   = "phaserand";
    inline constexpr const char* oscOctave      = "octave";
    inline constexpr const char* oscSemi        = "semi";
    inline constexpr const char* oscFine        = "fine";
    inline constexpr const char* oscToFilter    = "tofilter";
    inline constexpr const char* oscMode        = "mode";        // wavetable / sample
    inline constexpr const char* oscSampleLoop  = "smploop";
    inline constexpr const char* oscSampleRoot  = "smproot";

    // -- sub oscillator -----------------------------------------------------
    inline constexpr const char* subEnable      = "sub_enable";
    inline constexpr const char* subLevel       = "sub_level";
    inline constexpr const char* subPan         = "sub_pan";
    inline constexpr const char* subWave        = "sub_wave";
    inline constexpr const char* subOctave      = "sub_octave";
    inline constexpr const char* subToFilter    = "sub_tofilter";

    // -- noise --------------------------------------------------------------
    inline constexpr const char* noiseEnable    = "noise_enable";
    inline constexpr const char* noiseLevel     = "noise_level";
    inline constexpr const char* noisePan       = "noise_pan";
    inline constexpr const char* noiseColour    = "noise_colour";
    inline constexpr const char* noiseToFilter  = "noise_tofilter";

    // -- filter -------------------------------------------------------------
    inline constexpr const char* filterEnable   = "flt_enable";
    inline constexpr const char* filterType     = "flt_type";
    inline constexpr const char* filterCutoff   = "flt_cutoff";
    inline constexpr const char* filterReso     = "flt_reso";
    inline constexpr const char* filterDrive    = "flt_drive";
    inline constexpr const char* filterMix      = "flt_mix";
    inline constexpr const char* filterKeytrack = "flt_keytrack";

    // -- envelope (per index) ----------------------------------------------
    inline constexpr const char* envAttack      = "attack";
    inline constexpr const char* envHold        = "hold";
    inline constexpr const char* envDecay       = "decay";
    inline constexpr const char* envSustain     = "sustain";
    inline constexpr const char* envRelease     = "release";
    inline constexpr const char* envAttackCurve = "atkcurve";
    inline constexpr const char* envDecayCurve  = "deccurve";
    inline constexpr const char* envRelCurve    = "relcurve";

    // -- LFO (per index) ----------------------------------------------------
    inline constexpr const char* lfoShape       = "shape";
    inline constexpr const char* lfoRateHz      = "ratehz";
    inline constexpr const char* lfoRateSync    = "ratesync";        // tempo division index
    inline constexpr const char* lfoSyncMode    = "syncmode";        // free / tempo
    inline constexpr const char* lfoTrigger     = "trigger";         // trigger / envelope / free
    inline constexpr const char* lfoPhase       = "phase";
    inline constexpr const char* lfoRise        = "rise";
    inline constexpr const char* lfoSmooth      = "smooth";
    inline constexpr const char* lfoBipolar     = "bipolar";

    // -- modulation matrix (per slot) --------------------------------------
    inline constexpr const char* modEnable      = "on";
    inline constexpr const char* modSource      = "src";
    inline constexpr const char* modDest        = "dst";
    inline constexpr const char* modAmount      = "amt";
    inline constexpr const char* modBipolar     = "bip";
    inline constexpr const char* modVia         = "via";

    // -- effect slot (per index) -------------------------------------------
    inline constexpr const char* fxEnable       = "enable";
    inline constexpr const char* fxType         = "type";
    inline constexpr const char* fxMix          = "mix";
    inline constexpr const char* fxParamA       = "a";
    inline constexpr const char* fxParamB       = "b";
    inline constexpr const char* fxParamC       = "c";
}
