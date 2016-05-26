//--------------------------------------------------------------------------------------------------
// Music.cpp
//
// The music demo logic lives in this file
//
//--------------------------------------------------------------------------------------------------

#include "Music.h"
#include <cmath>
#include <conio.h>
#include <stdio.h>

static const float c_pi = 3.14159265359f;

float g_frequency = 0.0f;

//--------------------------------------------------------------------------------------------------
inline static float SawWave (float phase) {
    return phase * 2.0f - 1.0f;
}

//--------------------------------------------------------------------------------------------------
inline static float SineWave (float phase) {
    return std::sinf(phase * 2.0f * c_pi);
}

//--------------------------------------------------------------------------------------------------
inline static float SquareWave (float phase) {
    return phase >= 0.5f ? 1.0f : -1.0f;
}

//--------------------------------------------------------------------------------------------------
inline static float TriangleWave (float phase) {
    return std::abs(phase - 0.5f) * 4.0f - 1.0f;
}

//--------------------------------------------------------------------------------------------------
float NoteToFrequency (float fOctave, float fNote)
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
void GenerateAudioSamplesCallback (float *outputBuffer, size_t framesPerBuffer, size_t numChannels, float sampleRate) {
    static float phase = 0.5f;

    //static const float frequency = 500.0f;
    float phaseAdvance = g_frequency / sampleRate;

    for (size_t sample = 0; sample < framesPerBuffer; ++sample, outputBuffer += numChannels) {

        float value = SineWave(phase);
        phase += phaseAdvance;
        phase = std::fmod(phase, 1.0f);

        for (size_t channel = 0; channel < numChannels; ++channel)
            outputBuffer[channel] = value;
    }
}

//--------------------------------------------------------------------------------------------------
void MusicDemosMain () {

    printf("Main Menu:\n");

    bool exit = false;

    while (!exit) {
        switch (_getch()) {
            case '1': g_frequency = NoteToFrequency(4, 0); break;
            case '2': g_frequency = NoteToFrequency(4, 1); break;
            case '3': g_frequency = NoteToFrequency(4, 2); break;
            case '4': g_frequency = NoteToFrequency(4, 3); break;
            case '5': g_frequency = NoteToFrequency(4, 4); break;
            case '6': g_frequency = NoteToFrequency(4, 5); break;
            case '7': g_frequency = NoteToFrequency(4, 6); break;
            case '8': g_frequency = NoteToFrequency(4, 7); break;
            case '9': g_frequency = NoteToFrequency(4, 8); break;
            case '0': g_frequency = NoteToFrequency(4, 9); break;
            case 27: exit = true; break;
        }
    }
}

/*

TODO:
 * maybe have Main.cpp control the main loop, and have it call GetKeyboardState, then call up to the current demo with key up and key down messages.
  * this would allow us to support multiple key presses at once.
  * also need a system to return when it's time to exit i guess

 * need to make a system where you make a class for each demo.  make one demo that is the main menu.

 ! note in presentation that keyboards aren't great at multiple key presses at once. keyboardsareevil.com

*/