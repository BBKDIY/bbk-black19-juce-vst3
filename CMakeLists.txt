cmake_minimum_required(VERSION 3.22)

project(BBKBlack19 VERSION 1.0.0 LANGUAGES C CXX)

option(BBK_BUILD_DSP_TEST_ONLY "Build only the framework-independent DSP verification executable" OFF)

add_executable(BBKBlack19_DSPTest
    Tests/DSPTest.cpp
    Source/Black19Kernel.h
)
target_compile_features(BBKBlack19_DSPTest PRIVATE cxx_std_17)

if(BBK_BUILD_DSP_TEST_ONLY)
    return()
endif()

# Prefer a JUCE checkout placed directly in this project as ./JUCE.
# Otherwise fetch the pinned stable JUCE release automatically.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/JUCE/CMakeLists.txt")
    add_subdirectory(JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(
        JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.15
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(JUCE)
endif()

# IDENTITY TEST BUILD: renamed product/bundle/code so this installs
# alongside (not over) the real "BBK Black-19" plugin. The underlying
# target name BBKBlack19 is kept so the .cpp/.h sources compile unchanged.
juce_add_plugin(BBKBlack19
    VERSION 1.0.0
    COMPANY_NAME "BBK"
    BUNDLE_ID "com.bbk.identitytest"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE Bbk1
    PLUGIN_CODE Idt1
    FORMATS VST3
    PRODUCT_NAME "BBK IdentityTest"
    NEEDS_WEB_BROWSER FALSE
    NEEDS_CURL FALSE
)

target_sources(BBKBlack19 PRIVATE
    Source/Black19Kernel.h
    Source/PluginProcessor.h
    Source/PluginProcessor.cpp
    Source/PluginEditor.h
    Source/PluginEditor.cpp
)

target_compile_features(BBKBlack19 PRIVATE cxx_std_17)

target_compile_definitions(BBKBlack19 PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(BBKBlack19
    PRIVATE
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
