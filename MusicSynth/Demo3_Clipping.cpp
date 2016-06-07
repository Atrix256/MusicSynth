//--------------------------------------------------------------------------------------------------
// Demo3_Clipping.cpp
//
// Logic for the demo of the same name
//
//--------------------------------------------------------------------------------------------------

#include "DemoMgr.h"

namespace Demo3_Clipping {

    float   g_frequency = 0.0f;
    float   g_volumeAmplifier = 1.0f;

    enum EVoiceState {
        e_stopped,
        e_wantStart,
        e_started
    };

    EVoiceState g_voiceState = e_stopped;

    //--------------------------------------------------------------------------------------------------
    void OnInit() { }

    //--------------------------------------------------------------------------------------------------
    void OnExit() { }

    //--------------------------------------------------------------------------------------------------
    float SampleAudioSample(size_t age, SWavFile& sample, float sampleRate) {

        // handle the note dieing when it is done
        size_t sampleIndex = age*sample.m_numChannels;
        if (sampleIndex >= sample.m_numSamples) {
            g_voiceState = e_stopped;
            return 0.0f;
        }

        // calculate and apply an envelope to the sound samples
        float ageInSeconds = float(age) / sampleRate;
        float envelope = Envelope4Pt(
            ageInSeconds,
            0.0f, 0.0f,
            0.1f, 1.0f,
            sample.m_lengthSeconds - 0.1f, 1.0f,
            sample.m_lengthSeconds, 0.0f
        );

        // return the sample value multiplied by the envelope
        return sample.m_samples[sampleIndex] * envelope;
    }

    //--------------------------------------------------------------------------------------------------
    void GenerateAudioSamples (float *outputBuffer, size_t framesPerBuffer, size_t numChannels, float sampleRate) {
        static float phase = 0.0f;

        // handle the voice starting
        static size_t voiceStarted = 0;
        EVoiceState voiceState = g_voiceState;
        if (voiceState == e_wantStart) {
            g_voiceState = e_started;
            voiceState = e_started;
            voiceStarted = CDemoMgr::GetSampleClock();
        }

        // calculate how much our phase should change each sample
        float phaseAdvance = g_frequency / sampleRate;

        for (size_t sample = 0; sample < framesPerBuffer; ++sample, outputBuffer += numChannels) {

            // get the sine wave amplitude for this phase (angle)
            float value = SineWave(phase) * g_volumeAmplifier;

            // sample the voice if we should
            if (voiceState == e_started)
                value += SampleAudioSample(CDemoMgr::GetSampleClock() - voiceStarted + sample, g_sample_legend2, sampleRate) * g_volumeAmplifier;

            // advance the phase, making sure to stay within 0 and 1
            phase += phaseAdvance;
            phase = std::fmod(phase, 1.0f);

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

        // left alt for toggling voice
        if (key == -92) {
            if (g_voiceState == e_stopped)
                g_voiceState = e_wantStart;
            else
                g_voiceState = e_stopped;
        }

        switch (key) {
            // number row
            case '1': g_volumeAmplifier = 1.0f; break;
            case '2': g_volumeAmplifier = 2.0f; break;
            case '3': g_volumeAmplifier = 3.0f; break;
            case '4': g_volumeAmplifier = 4.0f; break;
            case '5': g_volumeAmplifier = 5.0f; break;
            case '6': g_volumeAmplifier = 6.0f; break;
            case '7': g_volumeAmplifier = 7.0f; break;
            case '8': g_volumeAmplifier = 8.0f; break;
            case '9': g_volumeAmplifier = 9.0f; break;
            case '0': g_volumeAmplifier = 20.0f; break;

            // QWERTY row
            case 'Q': g_frequency = NoteToFrequency(3, 0); break;
            case 'W': g_frequency = NoteToFrequency(3, 1); break;
            case 'E': g_frequency = NoteToFrequency(3, 2); break;
            case 'R': g_frequency = NoteToFrequency(3, 3); break;
            case 'T': g_frequency = NoteToFrequency(3, 4); break;
            case 'Y': g_frequency = NoteToFrequency(3, 5); break;
            case 'U': g_frequency = NoteToFrequency(3, 6); break;
            case 'I': g_frequency = NoteToFrequency(3, 7); break;
            case 'O': g_frequency = NoteToFrequency(3, 8); break;
            case 'P': g_frequency = NoteToFrequency(3, 9); break;
            case -37: g_frequency = NoteToFrequency(3, 10); break;

            // ASDF row
            case 'A': g_frequency = NoteToFrequency(2, 0); break;
            case 'S': g_frequency = NoteToFrequency(2, 1); break;
            case 'D': g_frequency = NoteToFrequency(2, 2); break;
            case 'F': g_frequency = NoteToFrequency(2, 3); break;
            case 'G': g_frequency = NoteToFrequency(2, 4); break;
            case 'H': g_frequency = NoteToFrequency(2, 5); break;
            case 'J': g_frequency = NoteToFrequency(2, 6); break;
            case 'K': g_frequency = NoteToFrequency(2, 7); break;
            case 'L': g_frequency = NoteToFrequency(2, 8); break;
            case -70: g_frequency = NoteToFrequency(2, 9); break;
            case -34: g_frequency = NoteToFrequency(2, 10); break;

            // ZXCV row
            case 'Z': g_frequency = NoteToFrequency(1, 0); break;
            case 'X': g_frequency = NoteToFrequency(1, 1); break;
            case 'C': g_frequency = NoteToFrequency(1, 2); break;
            case 'V': g_frequency = NoteToFrequency(1, 3); break;
            case 'B': g_frequency = NoteToFrequency(1, 4); break;
            case 'N': g_frequency = NoteToFrequency(1, 5); break;
            case 'M': g_frequency = NoteToFrequency(1, 6); break;
            case -68: g_frequency = NoteToFrequency(1, 7); break;
            case -66: g_frequency = NoteToFrequency(1, 8); break;
            case -65: g_frequency = NoteToFrequency(1, 9); break;
            case -95: g_frequency = NoteToFrequency(1, 10); break;  // right shift

            // left shift = low freq
            case 16: g_frequency = NoteToFrequency(0, 5); break;

            // left shift = low freq
            case -94: g_frequency = NoteToFrequency(0, 0); break;

            // silence
            case ' ': g_frequency = 0.0f; break;
            default: {
                return;
            }
        }

        printf("Frequency = %0.2fhz, Volume = %i%%\r\n", g_frequency, int(g_volumeAmplifier*100.0f));
    }

    //--------------------------------------------------------------------------------------------------
    void OnEnterDemo () {
        g_frequency = 0.0f;
        g_volumeAmplifier = 1.0f;
        g_voiceState = e_stopped;
        printf("Number keys to adjust volume and adjust clipping.\r\nLetter key to play different sine tones. Space to silence.\r\nMelody1 = ZMAM. left shift / control is super low frequency.\r\nleft alt for a voice sample.\r\n");
    }
}