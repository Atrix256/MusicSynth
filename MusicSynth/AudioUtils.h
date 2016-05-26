//--------------------------------------------------------------------------------------------------
// AudioUtils.h
//
// Helper functions
//
//--------------------------------------------------------------------------------------------------
#pragma once

#include <cmath>

static const float c_pi = 3.14159265359f;

//--------------------------------------------------------------------------------------------------
inline float SawWave(float phase) {
    return phase * 2.0f - 1.0f;
}

//--------------------------------------------------------------------------------------------------
inline float SineWave(float phase) {
    return std::sinf(phase * 2.0f * c_pi);
}

//--------------------------------------------------------------------------------------------------
inline float SquareWave(float phase) {
    return phase >= 0.5f ? 1.0f : -1.0f;
}

//--------------------------------------------------------------------------------------------------
inline float TriangleWave(float phase) {
    return std::abs(phase - 0.5f) * 4.0f - 1.0f;
}

//--------------------------------------------------------------------------------------------------
inline float NoteToFrequency(float fOctave, float fNote)
/*
Calculate the frequency of any note!
frequency = 440×(2^(n/12))

N=0 is A4
N=1 is A#4
etc...

notes go like so...
0  = A
1  = A#
2  = B
3  = C
4  = C#
5  = D
6  = D#
7  = E
8  = F
9  = F#
10 = G
11 = G#
*/
{
    return (float)(440 * pow(2.0, ((double)((fOctave - 4) * 12 + fNote)) / 12.0));
}

//--------------------------------------------------------------------------------------------------
inline float Lerp (float a, float b, float t) {
    return (b - a)*t + a;
}