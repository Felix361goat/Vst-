# A console runner for the engine tests.
#
# It compiles the engine sources directly rather than linking the plugin, so
# the tests never need a host, a window or the plugin client defines.

juce_add_console_app(NogSuiteTests
    PRODUCT_NAME "NOG Suite Tests")

target_sources(NogSuiteTests PRIVATE
    ${NOG_ENGINE_SOURCES}
    ${CMAKE_CURRENT_SOURCE_DIR}/EngineTests.cpp)

target_include_directories(NogSuiteTests PRIVATE ${CMAKE_SOURCE_DIR}/Source)

target_compile_definitions(NogSuiteTests PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_STANDALONE_APPLICATION=1)

target_link_libraries(NogSuiteTests PRIVATE
    juce::juce_audio_processors
    juce::juce_audio_utils
    juce::juce_dsp
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags)

add_test(NAME NogSuiteTests COMMAND NogSuiteTests)
