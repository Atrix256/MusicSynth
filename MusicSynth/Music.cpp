//--------------------------------------------------------------------------------------------------
// Music.cpp
//
// The music demo logic lives in this file
//
//--------------------------------------------------------------------------------------------------

#include "Music.h"
#include <cmath>
#include <Windows.h>  // for Sleep().  Get rid of when Sleep is no longer needed.

static const float c_pi = 3.14159265359f;

//--------------------------------------------------------------------------------------------------
inline static float SawWave(float phase) {
    return phase * 2.0f - 1.0f;
}

//--------------------------------------------------------------------------------------------------
inline static float SineWave(float phase) {
    return std::sinf(phase * 2.0f * c_pi);
}

//--------------------------------------------------------------------------------------------------
inline static float SquareWave(float phase) {
    return phase >= 0.5f ? 1.0f : -1.0f;
}

//--------------------------------------------------------------------------------------------------
inline static float TriangleWave(float phase) {
    return std::abs(phase - 0.5f) * 4.0f - 1.0f;
}

//--------------------------------------------------------------------------------------------------
void GenerateAudioSamplesCallback(float *outputBuffer, size_t framesPerBuffer, size_t numChannels, float sampleRate) {
    static float phase = 0.5f;

    static const float frequency = 500.0f;
    static const float phaseAdvance = frequency / sampleRate;

    for (size_t sample = 0; sample < framesPerBuffer; ++sample, outputBuffer += numChannels) {

        float value = SineWave(phase);
        phase += phaseAdvance;
        phase = std::fmod(phase, 1.0f);

        for (size_t channel = 0; channel < numChannels; ++channel)
            outputBuffer[channel] = value;
    }
}

//--------------------------------------------------------------------------------------------------
void MusicDemosMain() {
    // sleep for 5 seconds, until we get a propper menu and related logic in for doing demo stuff.
    Sleep(5000);
}