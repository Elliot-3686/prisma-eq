cmake_minimum_required(VERSION 3.25)

# ─── Product identity: rename the plugin here, and only here ────────────────
set(PRISMA_PRODUCT_NAME     "Prisma EQ")
set(PRISMA_COMPANY_NAME     "Prisma Audio")
set(PRISMA_MANUFACTURER     Prsm)           # 4 chars, at least one upper case
set(PRISMA_PLUGIN_CODE      PrEq)           # 4 chars, unique per plugin
set(PRISMA_BUNDLE_ID        com.prisma-audio.prisma-eq)
set(PRISMA_VERSION          0.1.0)

project(PrismaEQ VERSION ${PRISMA_VERSION} LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

option(PRISMA_BUILD_PLUGIN "Build the JUCE plugin (AU/VST3/Standalone)" ON)
option(PRISMA_BUILD_TESTS  "Build the DSP unit tests (Catch2)" ON)
option(PRISMA_COPY_PLUGIN  "Copy the plugin to the user plug-in folders after build" OFF)

include(FetchContent)

# ─── DSP core: plain C++20, no JUCE → tests build in seconds ────────────────
add_library(prisma_dsp STATIC
    Source/DSP/DspInfo.cpp
    Source/DSP/AnalogPrototype.cpp
    Source/DSP/BiquadDesign.cpp
    Source/DSP/EqEngine.cpp
)
target_include_directories(prisma_dsp PUBLIC Source)
if(MSVC)
    target_compile_options(prisma_dsp PRIVATE /W4)
else()
    target_compile_options(prisma_dsp PRIVATE -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wno-sign-conversion)
endif()

# ─── Plugin ─────────────────────────────────────────────────────────────────
if(PRISMA_BUILD_PLUGIN)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG        8.0.15
        GIT_SHALLOW    TRUE)
    FetchContent_MakeAvailable(JUCE)

    set(PRISMA_FORMATS VST3 Standalone)
    if(APPLE)
        list(APPEND PRISMA_FORMATS AU)
    endif()

    juce_add_plugin(PrismaEQ
        PRODUCT_NAME                "${PRISMA_PRODUCT_NAME}"
        COMPANY_NAME                "${PRISMA_COMPANY_NAME}"
        BUNDLE_ID                   ${PRISMA_BUNDLE_ID}
        PLUGIN_MANUFACTURER_CODE    ${PRISMA_MANUFACTURER}
        PLUGIN_CODE                 ${PRISMA_PLUGIN_CODE}
        FORMATS                     ${PRISMA_FORMATS}
        IS_SYNTH                    FALSE
        NEEDS_MIDI_INPUT            FALSE
        NEEDS_MIDI_OUTPUT           FALSE
        IS_MIDI_EFFECT              FALSE
        EDITOR_WANTS_KEYBOARD_FOCUS TRUE
        AU_MAIN_TYPE                kAudioUnitType_Effect
        VST3_CATEGORIES             Fx EQ
        COPY_PLUGIN_AFTER_BUILD     ${PRISMA_COPY_PLUGIN}
        MICROPHONE_PERMISSION_ENABLED TRUE
        MICROPHONE_PERMISSION_TEXT  "Prisma EQ (version autonome) traite le son de l'entrée audio choisie.")

    target_sources(PrismaEQ PRIVATE
        Source/Params/ParameterLayout.cpp
        Source/Plugin/PluginProcessor.cpp
        Source/Plugin/PluginEditor.cpp
        Source/UI/ResponseDisplay.cpp
        Source/UI/Panels.cpp)

    target_compile_definitions(PrismaEQ PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_STRICT_REFCOUNTEDPOINTER=1
        PRISMA_VERSION_STRING="${PRISMA_VERSION}")

    target_link_libraries(PrismaEQ
        PRIVATE
            prisma_dsp
            juce::juce_audio_utils
            juce::juce_dsp
        PUBLIC
            juce::juce_recommended_config_flags
            juce::juce_recommended_lto_flags
            juce::juce_recommended_warning_flags)
endif()

# ─── Tests ──────────────────────────────────────────────────────────────────
if(PRISMA_BUILD_TESTS)
    FetchContent_Declare(Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG        v3.16.1
        GIT_SHALLOW    TRUE)
    FetchContent_MakeAvailable(Catch2)

    add_executable(prisma_tests
        Tests/DspBasicsTests.cpp
        Tests/EqTests.cpp)
    target_link_libraries(prisma_tests PRIVATE prisma_dsp Catch2::Catch2WithMain)

    enable_testing()
    list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
    include(Catch)
    catch_discover_tests(prisma_tests)
endif()
