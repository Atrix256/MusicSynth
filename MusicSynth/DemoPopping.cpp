//--------------------------------------------------------------------------------------------------
// DemoPopping.cpp
//
// Logic for the demo of the same name
//
//--------------------------------------------------------------------------------------------------

#include "DemoMgr.h"

namespace DemoPopping {

    enum EMode {
        e_silence,
        e_notesPop,
        e_notesNoPop,
        e_notesSlideNoPop,
        e_samplePop,
        e_sampleNoPop
    };

    const char* ModeToString (EMode mode) {
        switch (mode) {
            case e_silence: return "Silence";
            case e_notesPop: return "Popping Notes";
            case e_notesNoPop: return "Notes Without Popping";
            case e_notesSlideNoPop: return "Sliding Notes";
            case e_samplePop: return "Popping Sample";
            case e_sampleNoPop: return "Sample Without Popping";
        }
        return "???";
    }

    EMode g_mode = e_silence;

    //--------------------------------------------------------------------------------------------------
    void OnInit() { }

    //--------------------------------------------------------------------------------------------------
    void OnExit() { }

    //--------------------------------------------------------------------------------------------------
    void SampleAudioSamples(float *outputBuffer, size_t framesPerBuffer, size_t numChannels, float sampleRate, size_t &baseIndex, bool pop) {

        SWavFile& sound = g_sample_dreams;

        // fill the buffer
        for (size_t sample = 0; sample < framesPerBuffer; ++sample, outputBuffer += numChannels, ++baseIndex) {

            // handle the sound dieing when it is done
            size_t sampleIndex = baseIndex*sound.m_numChannels;
            if (sampleIndex >= sound.m_numSamples) {
                g_mode = e_silence;
                for (size_t channel = 0; channel < numChannels; ++channel)
                    outputBuffer[channel] = 0.0f;
                return;
            }

            // calculate and apply an envelope to the sound samples
            float envelope = 1.0f;
            if (!pop) {
                const float c_envelopeTime = 0.03f;
                float ageInSeconds = float(baseIndex) / sampleRate;
                envelope = Envelope4Pt(
                    ageInSeconds,
                    0.0f, 0.0f,
                    c_envelopeTime, 1.0f,
                    sound.m_lengthSeconds - c_envelopeTime, 1.0f,
                    sound.m_lengthSeconds, 0.0f
                );
            }

            // return the sample value multiplied by the envelope
            float value = sound.m_samples[sampleIndex] * envelope;

            // copy the value to all audio channels
            for (size_t channel = 0; channel < numChannels; ++channel)
                outputBuffer[channel] = value;
        }

    }

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

        // sample our audio samples if we should
        if (mode == e_samplePop || mode == e_sampleNoPop) {
            SampleAudioSamples(outputBuffer, framesPerBuffer, numChannels, sampleRate, sampleIndex, mode == e_samplePop);
            return;
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
                case e_notesNoPop: {
                    // calculate an envelope for the beginning and end to avoid pops there
                    float envelope = 1.0f;
                    if (timeInSeconds < 0.05f)
                        envelope = Lerp(0.0, 1.0, timeInSeconds / 0.05f);
                    else if (timeInSeconds > 1.95)
                        envelope = Lerp(1.0, 0.0, std::fmin((timeInSeconds - 1.95f) / 0.05f, 1.0f));

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

                    // multiply in the envelope to avoid popping at the beginning and end
                    value *= envelope;

                    // advance the phase, making sure to stay within 0 and 1
                    phase += phaseAdvance;
                    phase = std::fmod(phase, 1.0f);
                    break;
                }
                case e_notesSlideNoPop: {
                    // calculate an envelope for the beginning and end to avoid pops there
                    float envelope = 1.0f;
                    if (timeInSeconds < 0.05f)
                        envelope = Lerp(0.0, 1.0, timeInSeconds / 0.05f);
                    else if (timeInSeconds > 1.95)
                        envelope = Lerp(1.0, 0.0, std::fmin((timeInSeconds - 1.95f) / 0.05f, 1.0f));

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

                    // multiply in the envelope to avoid popping at the beginning and end
                    value *= envelope;

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
    void ReportParams () {
        printf("%s\r\n", ModeToString(g_mode));
    }

    //--------------------------------------------------------------------------------------------------
    void OnKey (char key, bool pressed) {

        // only listen to key down events
        if (!pressed)
            return;

        // switch mode based on key press
        switch (key) {
            case '1': g_mode = e_notesPop; ReportParams(); break;
            case '2': g_mode = e_notesNoPop; ReportParams(); break;
            case '3': g_mode = e_notesSlideNoPop; ReportParams(); break;
            case '4': g_mode = e_samplePop; ReportParams(); break;
            case '5': g_mode = e_sampleNoPop; ReportParams(); break;
        }
    }

    //--------------------------------------------------------------------------------------------------
    void OnEnterDemo () {
        g_mode = e_silence;
        printf("1 = notes with pop.\r\n2 = notes without pop.\r\n3 = note slide without pop.\r\n4 = sample with pop\r\n5 = sample without pop\r\n");
        printf("\r\nInstructions:\r\n");
        printf("#1 and point out popping. #2 no popping. #3 shows what you can do.\r\n");
        printf("#4/#5 to show how envelopes can help.\r\n");
    }
}