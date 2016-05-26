//--------------------------------------------------------------------------------------------------
// Demo2_Popping.cpp
//
// Logic for the demo of the same name
//
//--------------------------------------------------------------------------------------------------

#include "DemoMgr.h"

namespace Demo2_Popping {

    enum EMode {
        e_silence,
        e_notesPop,
        e_notesSlidePop,
        e_notesNoPop,
        e_notesSlideNoPop,
    };

    EMode g_mode = e_silence;

    //--------------------------------------------------------------------------------------------------
    void GenerateAudioSamples (float *outputBuffer, size_t framesPerBuffer, size_t numChannels, float sampleRate) {

        // state information stored as statics
        static EMode mode = e_silence;
        static float phase = 0.0f;
        static size_t sampleIndex = 0;

        // switch modes if we should
        EMode newMode = g_mode;
        if (newMode != mode) {
            mode = newMode;
            phase = 0.0f;
            sampleIndex = 0;
        }

        // calculate how many audio samples happen in 1/4 of a second
        const size_t c_quarterSecond = size_t(sampleRate) / 4;

        for (size_t sample = 0; sample < framesPerBuffer; ++sample, outputBuffer += numChannels, ++sampleIndex) {

            // calculate a floating point time in seconds
            float timeInSeconds = float(sampleIndex) / sampleRate;

            // figure out how many quarter seconds have elapsed
            size_t quarterSeconds = size_t(timeInSeconds*4.0f);

            // calculate how far we are into our current quarter second chunk of time
            float quarterSecondsPercent = std::fmodf(timeInSeconds*4.0f, 1.0f);

            // go back to silence after 2 seconds
            if (quarterSeconds > 7)
                g_mode = e_silence;

            // figure out what our current audio sample should be, based on time and our behavior mode
            float value = 0.0f;
            switch (mode) {
                case e_notesPop: {
                    // play a different note depending on which quarter second we are in
                    float frequency = 0.0f;
                    switch (quarterSeconds) {
                        case 0: frequency = NoteToFrequency(3, 0); break;
                        case 1: frequency = NoteToFrequency(3, 1); break;
                        case 2: frequency = NoteToFrequency(3, 3); break;
                        case 3: frequency = NoteToFrequency(3, 1); break;
                        case 4: frequency = NoteToFrequency(3, 0); break;
                        case 5: frequency = NoteToFrequency(3, 1); break;
                        case 6: frequency = NoteToFrequency(3, 5); break;
                        case 7: frequency = NoteToFrequency(3, 1); break;
                    }

                    // calculate the sine value based entirely on time and frequency
                    value = std::sinf(timeInSeconds*frequency*2.0f*c_pi);
                    break;
                }
                case e_notesSlidePop: {
                    // slide between different notes depending on which quarter second we are in
                    float frequency = 0.0f;
                    switch (quarterSeconds) {
                        case 0: frequency = Lerp(NoteToFrequency(3, 0), NoteToFrequency(3, 1), quarterSecondsPercent); break;
                        case 1: frequency = Lerp(NoteToFrequency(3, 1), NoteToFrequency(3, 3), quarterSecondsPercent); break;
                        case 2: frequency = Lerp(NoteToFrequency(3, 3), NoteToFrequency(3, 1), quarterSecondsPercent); break;
                        case 3: frequency = Lerp(NoteToFrequency(3, 1), NoteToFrequency(3, 0), quarterSecondsPercent); break;
                        case 4: frequency = Lerp(NoteToFrequency(3, 0), NoteToFrequency(3, 1), quarterSecondsPercent); break;
                        case 5: frequency = Lerp(NoteToFrequency(3, 1), NoteToFrequency(3, 5), quarterSecondsPercent); break;
                        case 6: frequency = Lerp(NoteToFrequency(3, 5), NoteToFrequency(3, 1), quarterSecondsPercent); break;
                        case 7: frequency = Lerp(NoteToFrequency(3, 1), NoteToFrequency(3, 0), quarterSecondsPercent); break;
                    }

                    // calculate the sine value based entirely on time and frequency
                    value = std::sinf(timeInSeconds*frequency*2.0f*c_pi);
                    break;
                }
                case e_notesNoPop: {
                    // play a different note depending on which quarter second we are in
                    float frequency = 0.0f;
                    switch (quarterSeconds) {
                        case 0: frequency = NoteToFrequency(3, 0); break;
                        case 1: frequency = NoteToFrequency(3, 1); break;
                        case 2: frequency = NoteToFrequency(3, 3); break;
                        case 3: frequency = NoteToFrequency(3, 1); break;
                        case 4: frequency = NoteToFrequency(3, 0); break;
                        case 5: frequency = NoteToFrequency(3, 1); break;
                        case 6: frequency = NoteToFrequency(3, 5); break;
                        case 7: frequency = NoteToFrequency(3, 1); break;
                    }

                    // calculate how much to advance our phase for this frequency
                    float phaseAdvance = frequency / sampleRate;

                    // calculate the sine value based entirely on phase
                    value = std::sinf(phase * 2.0f * c_pi);

                    // advance the phase, making sure to stay within 0 and 1
                    phase += phaseAdvance;
                    phase = std::fmod(phase, 1.0f);
                    break;
                }
                case e_notesSlideNoPop: {
                    // slide between different notes depending on which quarter second we are in
                    float frequency = 0.0f;
                    switch (quarterSeconds) {
                        case 0: frequency = Lerp(NoteToFrequency(3, 0), NoteToFrequency(3, 1), quarterSecondsPercent); break;
                        case 1: frequency = Lerp(NoteToFrequency(3, 1), NoteToFrequency(3, 3), quarterSecondsPercent); break;
                        case 2: frequency = Lerp(NoteToFrequency(3, 3), NoteToFrequency(3, 1), quarterSecondsPercent); break;
                        case 3: frequency = Lerp(NoteToFrequency(3, 1), NoteToFrequency(3, 0), quarterSecondsPercent); break;
                        case 4: frequency = Lerp(NoteToFrequency(3, 0), NoteToFrequency(3, 1), quarterSecondsPercent); break;
                        case 5: frequency = Lerp(NoteToFrequency(3, 1), NoteToFrequency(3, 5), quarterSecondsPercent); break;
                        case 6: frequency = Lerp(NoteToFrequency(3, 5), NoteToFrequency(3, 1), quarterSecondsPercent); break;
                        case 7: frequency = Lerp(NoteToFrequency(3, 1), NoteToFrequency(3, 0), quarterSecondsPercent); break;
                    }

                    // calculate how much to advance our phase for this frequency
                    float phaseAdvance = frequency / sampleRate;

                    // calculate the sine value based entirely on phase
                    value = std::sinf(phase * 2.0f * c_pi);

                    // advance the phase, making sure to stay within 0 and 1
                    phase += phaseAdvance;
                    phase = std::fmod(phase, 1.0f);
                    break;
                }
            }

            // copy the value to all audio channels
            for (size_t channel = 0; channel < numChannels; ++channel)
                outputBuffer[channel] = value;
        }
    }

    //--------------------------------------------------------------------------------------------------
    void OnKey (char key, bool pressed) {

        // only listen to key down events
        if (!pressed)
            return;

        // switch mode based on key press
        switch (key) {
            case '1': g_mode = e_notesPop; break;
            case '2': g_mode = e_notesSlidePop; break;
            case '3': g_mode = e_notesNoPop; break;
            case '4': g_mode = e_notesSlideNoPop; break;
        }
    }

    //--------------------------------------------------------------------------------------------------
    void OnEnterDemo () {
        g_mode = e_silence;
        printf("\r\n\r\nEntering Demo: %s\r\n1 = notes with pop.\r\n2 = note slide with pop.\r\n3 = notes without pop.\r\n4 = note slide without pop.\r\n\r\n", s_demoName);
    }
}

/*

TODO:
* note slide pop sounds wrong, can you explain why?  maybe record a wav and look at it to see if there's anything obviously wrong
 * i don't think the fact that we are working based on time is enough to explain it.

*/