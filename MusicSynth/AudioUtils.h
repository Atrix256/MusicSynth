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

//--------------------------------------------------------------------------------------------------
inline float Envelope2Pt (
    float time,
    float time0, float volume0,
    float time1, float volume1
) {
    /*
    This function does a "line" type envelope.

      v1
       *
      / 
     /  
  v0*   
    +--+
    t0 t1

    */

    // if before envelope starts, return volume0
    if (time < time0)
        return volume0;
    
    // if after envelope ends, return volume1
    if (time > time1)
        return volume1;

    // else between time0 and time1, lerp between volume0 and volume1
    float percent = (time - time0) / (time1 - time0);
    return Lerp(volume0, volume1, percent);
}

//--------------------------------------------------------------------------------------------------
inline float Envelope3Pt (
    float time,
    float time0, float volume0,
    float time1, float volume1,
    float time2, float volume2
) {
    /*
    This function does a "triangle" type envelope.

      v1
       *
      / \
     /   \
  v0*     *v2
    +--+--+
    t0 t1 t2

    */

    // if before envelope starts, return volume0
    if (time < time0)
        return volume0;
    
    // if after envelope ends, return volume2
    if (time > time2)
        return volume2;

    // if between time0 and time1, lerp between volume0 and volume1
    if (time < time1) {
        float percent = (time - time0) / (time1 - time0);
        return Lerp(volume0, volume1, percent);
    }

    // else between time1 and time2, lerp between volume1 and volume2
    float percent = (time - time1) / (time2 - time1);
    return Lerp(volume1, volume2, percent);
}

//--------------------------------------------------------------------------------------------------
inline float Envelope4Pt (
    float time,
    float time0, float volume0,
    float time1, float volume1,
    float time2, float volume2,
    float time3, float volume3
) {
    /*
    This function does a "trapezoid" type envelope.

      v1  v2
       *--*
      /    \
     /      \
  v0*        *v3
    +--+--+--+
    t0 t1 t2 t3

    */

    // if before envelope starts, return volume0
    if (time < time0)
        return volume0;
    
    // if after envelope ends, return volume3
    if (time > time3)
        return volume3;

    // if between time0 and time1, lerp between volume0 and volume1
    if (time < time1) {
        float percent = (time - time0) / (time1 - time0);
        return Lerp(volume0, volume1, percent);
    }

    // else if between time1 and time2, lerp between volume1 and volume2
    if (time < time2) {
        float percent = (time - time1) / (time2 - time1);
        return Lerp(volume1, volume2, percent);
    }
    
    // else between time2 and time3, lerp between volume2 and volume3
    float percent = (time - time2) / (time3 - time2);
    return Lerp(volume2, volume3, percent);
}