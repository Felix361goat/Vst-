# Source lists, shared by the plugin target and the test runner.
#
# NOG_ENGINE_SOURCES is everything that can be built and tested without the
# JUCE plugin client - no JucePlugin_* defines, no host. Keeping that boundary
# sharp is what lets the unit tests run the real synthesis code rather than a
# stand-in for it.
#
# NOG_PLUGIN_SOURCES is the rest: the processor, the editor and anything that
# needs the plugin's identity or its window.

set(NOG_SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR})

set(NOG_ENGINE_SOURCES
    ${NOG_SOURCE_DIR}/Params/ParameterLayout.cpp
    ${NOG_SOURCE_DIR}/Params/ParameterStore.cpp
    ${NOG_SOURCE_DIR}/Modulation/ModMatrix.cpp
    ${NOG_SOURCE_DIR}/DSP/Oscillator.cpp
    ${NOG_SOURCE_DIR}/DSP/Wavetable.cpp
    ${NOG_SOURCE_DIR}/DSP/WavetableBank.cpp
    ${NOG_SOURCE_DIR}/Engine/Voice.cpp
    ${NOG_SOURCE_DIR}/Engine/SynthEngine.cpp
    ${NOG_SOURCE_DIR}/FX/FXChain.cpp
)

set(NOG_PLUGIN_SOURCES
    ${NOG_SOURCE_DIR}/PluginProcessor.cpp
    ${NOG_SOURCE_DIR}/PluginEditor.cpp
    ${NOG_SOURCE_DIR}/State/PresetManager.cpp
    ${NOG_SOURCE_DIR}/UI/NogLookAndFeel.cpp
    ${NOG_SOURCE_DIR}/UI/Widgets.cpp
    ${NOG_SOURCE_DIR}/UI/Panels/SynthPanels.cpp
    ${NOG_SOURCE_DIR}/UI/Panels/ModulatorPanels.cpp
    ${NOG_SOURCE_DIR}/UI/Panels/MatrixPanel.cpp
    ${NOG_SOURCE_DIR}/UI/Panels/FxPanel.cpp
    ${NOG_SOURCE_DIR}/UI/Panels/TopBar.cpp
)
