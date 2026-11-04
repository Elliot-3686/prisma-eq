#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace prisma::params
{
    constexpr int kNumBands = 24;

    /// Band shapes. Order is part of the saved state: append only, never reorder.
    enum class Shape { Bell, LowShelf, LowCut, HighShelf, HighCut, Notch, BandPass, TiltShelf, FlatTilt, AllPass };
    enum class Placement { Stereo, Left, Right, Mid, Side };
    enum class ProcessingMode { ZeroLatency, NaturalPhase, LinearPhase };

    const juce::StringArray& shapeNames();
    const juce::StringArray& placementNames();

    /// Stable parameter IDs ("b07_freq"): bands are never renumbered, so host
    /// automation stays valid when a band is deleted.
    juce::String bandId (int bandIndex, const char* name);

    namespace id
    {
        // per band (use with bandId)
        inline constexpr const char* used      = "used";
        inline constexpr const char* enabled   = "on";
        inline constexpr const char* shape     = "shape";
        inline constexpr const char* slope     = "slope";
        inline constexpr const char* brickwall = "brick";
        inline constexpr const char* freq      = "freq";
        inline constexpr const char* gain      = "gain";
        inline constexpr const char* q         = "q";
        inline constexpr const char* placement = "place";
        inline constexpr const char* gainQ     = "gainq";

        // global
        inline constexpr const char* bypass      = "bypass";
        inline constexpr const char* mode        = "mode";
        inline constexpr const char* resolution  = "resolution";
        inline constexpr const char* outGain     = "outGain";
        inline constexpr const char* outPan      = "outPan";
        inline constexpr const char* panMode     = "panMode";
        inline constexpr const char* phaseInvert = "phaseInvert";
        inline constexpr const char* autoGain    = "autoGain";
        inline constexpr const char* gainScale   = "gainScale";
        inline constexpr const char* character   = "character";
    }

    constexpr float kMinFreq = 5.0f, kMaxFreq = 30000.0f;
    constexpr float kMinQ = 0.025f, kMaxQ = 40.0f;
    constexpr float kMaxGain = 30.0f;
    constexpr float kMaxSlope = 96.0f;
    constexpr float kOutGainMin = -60.0f;   // shown as −∞
    constexpr float kOutGainMax = 36.0f;

    juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
}
