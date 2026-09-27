cmake_minimum_required(VERSION 3.22)
project(DelayLamaModern VERSION 1.0.0)

include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.9
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(JUCE)
juce_add_binary_data(DelayLamaAssets SOURCES
    Assets/facematrix_8bit.png
    Assets/background_8bit.png
    Assets/splash_v1.png
    Assets/knoplinksstrip_8bit.png
    Assets/knoprechtsstrip_8bit.png
)

juce_add_plugin(DelayLamaModern
    COMPANY_NAME "Personal Recreation"
    BUNDLE_ID "local.personal.delaylamamodern"
    PLUGIN_MANUFACTURER_CODE DLMO
    PLUGIN_CODE DLM1
    FORMATS AU VST3 Standalone
    PRODUCT_NAME "Delay Lama Modern"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    AU_SANDBOX_SAFE TRUE
    COPY_PLUGIN_AFTER_BUILD FALSE
)

target_sources(DelayLamaModern PRIVATE Source/PluginProcessor.cpp Source/PluginEditor.cpp)
target_compile_features(DelayLamaModern PRIVATE cxx_std_17)
target_compile_definitions(DelayLamaModern PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)
target_link_libraries(DelayLamaModern PRIVATE DelayLamaAssets juce::juce_audio_utils juce::juce_dsp)
