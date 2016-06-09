//--------------------------------------------------------------------------------------------------
// DemoStereo.cpp
//
// Logic for the demo of the same name
//
//--------------------------------------------------------------------------------------------------

#include "DemoMgr.h"
#include "AudioEffects.h"
#include <algorithm>

namespace DemoStereo {

    struct SNote {
        SNote(float frequency)
            : m_frequency(frequency)
            , m_age(0)
            , m_dead(false)
            , m_releaseAge(0)
            , m_phase(0.0f) {}

        float       m_frequency;
        size_t      m_age;
        bool        m_dead;
        size_t      m_releaseAge;
        float       m_phase;
    };

    std::vector<SNote>  g_notes;
    std::mutex          g_notesMutex;
    bool                g_rotateSound;
    bool                g_pingPongDelay;
    bool                g_cymbalsOn;
    bool                g_voiceOn;

    //--------------------------------------------------------------------------------------------------
    void OnInit() { }

    //--------------------------------------------------------------------------------------------------
    void OnExit() { }

    //--------------------------------------------------------------------------------------------------
    inline float GenerateNoteSample (SNote& note, float sampleRate) {

        float c_decayTime = 1.5f;
        size_t c_numHarmonics = 10;

        // calculate our age in seconds and advance our age in samples, by 1 sample
        float ageInSeconds = float(note.m_age) / sampleRate;
        ++note.m_age;

        // handle the note dieing
        if (ageInSeconds > c_decayTime)
        {
            note.m_dead = true;
            return 0.0;
        }

        // add our harmonics together
        float ret = 0.0f;
        for (size_t index = 1; index <= c_numHarmonics; ++index) {
            float envelope = Envelope4Pt(
                ageInSeconds * float(index),
                0.0f, 0.0f,
                c_decayTime*0.05f, 1.0f,
                c_decayTime*0.10f, 0.6f,
                c_decayTime, 0.0f
            );
            float phase = std::fmodf(note.m_phase * float(index) , 1.0f);
            //ret += SineWave(phase) * envelope;
            ret += SineWave(phase) * envelope;
        }

        // advance phase
        note.m_phase = std::fmodf(note.m_phase + note.m_frequency / sampleRate, 1.0f);

        // return the value
        return ret;
    }

    //--------------------------------------------------------------------------------------------------
    void GenerateAudioSamples (float *outputBuffer, size_t framesPerBuffer, size_t numChannels, float sampleRate) {

        // handle effect params
        static SPingPongDelayEffect delayEffect;
        bool rotateSound = g_rotateSound;
        static bool wasDelayOn = false;
        bool isDelayOn = g_pingPongDelay;
        if (isDelayOn != wasDelayOn) {
            delayEffect.SetEffectParams(0.33f, sampleRate, numChannels, 0.0625f);
            wasDelayOn = isDelayOn;
        }

        // handle playing samples
        static bool cymbalsWereOn = false;
        static size_t cymbalsStarted = 0;
        bool cymbalsAreOn = g_cymbalsOn;
        if (cymbalsWereOn != cymbalsAreOn) {
            cymbalsWereOn = cymbalsAreOn;
            cymbalsStarted = CDemoMgr::GetSampleClock();
        }
        static bool voiceWasOn = false;
        static size_t voiceStarted = 0;
        bool voiceIsOn = g_voiceOn;
        if (voiceWasOn != voiceIsOn) {
            voiceWasOn = voiceIsOn;
            voiceStarted = CDemoMgr::GetSampleClock();
        }

        // get a lock on our notes vector
        std::lock_guard<std::mutex> guard(g_notesMutex);

        // for every sample in our output buffer
        for (size_t sample = 0; sample < framesPerBuffer; ++sample, outputBuffer += numChannels) {
            
            // add up all notes to get the final value.
            float valueMono = 0.0f;
            std::for_each(
                g_notes.begin(),
                g_notes.end(),
                [&valueMono, sampleRate](SNote& note) {
                    valueMono += GenerateNoteSample(note, sampleRate) * 0.25f;
                }
            );

            // sample the samples if we should
            if (cymbalsAreOn) {
                size_t sampleIndex = (CDemoMgr::GetSampleClock() + sample - cymbalsStarted) * numChannels;
                if (sampleIndex < g_sample_cymbal.m_numSamples) {
                    valueMono += g_sample_cymbal.m_samples[sampleIndex] * 2.0f;
                }
                else {
                    g_cymbalsOn = false;
                }
            }
            if (voiceIsOn) {
                size_t sampleIndex = (CDemoMgr::GetSampleClock() + sample - voiceStarted) * numChannels;
                if (sampleIndex < g_sample_legend2.m_numSamples) {
                    valueMono += g_sample_legend2.m_samples[sampleIndex] * 2.0f;
                }
                else {
                    g_voiceOn = false;
                }
            }

            // split the mono sound into a stereo sound
            float valueLeft = valueMono;
            float valueRight = valueMono;

            // if sound rotation is on, make some sine/cosine tones to simulate 3d
            if (rotateSound) {
                float timeInSeconds = float(CDemoMgr::GetSampleClock() + sample) / sampleRate;
                valueLeft *= std::sinf(timeInSeconds*0.25f*2.0f*c_pi) * 0.45f + 0.55f;
                valueRight *= std::cosf(timeInSeconds*0.25f*2.0f*c_pi) * 0.45f + 0.55f;
            }

            // do ping pong delay if we should
            if (isDelayOn && numChannels >= 2) {
                float echoLeft, echoRight;
                delayEffect.AddSample(valueMono, echoLeft, echoRight);
                valueLeft += echoLeft;
                valueRight += echoRight;
            }

            // copy the values to all audio channels
            for (size_t channel = 0; channel < numChannels; ++channel) {
                if (channel % 2 == 0)
                    outputBuffer[channel] = valueLeft;
                else
                    outputBuffer[channel] = valueRight;
            }
        }

        // remove notes that have died
        auto iter = std::remove_if(
            g_notes.begin(),
            g_notes.end(),
            [] (const SNote& note) {
                return note.m_dead;
            }
        );

        if (iter != g_notes.end())
            g_notes.erase(iter);
    }

    //--------------------------------------------------------------------------------------------------
    void ReportParams() {
        printf("Rotate Sound: %s, Ping Pong Delay: %s\r\n", g_rotateSound ? "On" : "Off", g_pingPongDelay ? "On" : "Off");
    }

    //--------------------------------------------------------------------------------------------------
    void OnKey (char key, bool pressed) {

        // nothing to do on key release
        if (!pressed)
            return;

        // pressing 1 toggles music
        if (key == '1') {
            g_rotateSound = !g_rotateSound;
            ReportParams();
            return;
        }

        // pressing 2 toggles ping pong delay
        if (key == '2') {
            g_pingPongDelay = !g_pingPongDelay;
            ReportParams();
            return;
        }

        // samples
        if (key == '3') {
            g_cymbalsOn = !g_cymbalsOn;
            return;
        }
        if (key == '4') {
            g_voiceOn = !g_voiceOn;
            return;
        }

        // figure out what frequency to play
        float frequency = 0.0f;
        switch (key) {
            // QWERTY row
            case 'Q': frequency = NoteToFrequency(3, 0); break;
            case 'W': frequency = NoteToFrequency(3, 1); break;
            case 'E': frequency = NoteToFrequency(3, 2); break;
            case 'R': frequency = NoteToFrequency(3, 3); break;
            case 'T': frequency = NoteToFrequency(3, 4); break;
            case 'Y': frequency = NoteToFrequency(3, 5); break;
            case 'U': frequency = NoteToFrequency(3, 6); break;
            case 'I': frequency = NoteToFrequency(3, 7); break;
            case 'O': frequency = NoteToFrequency(3, 8); break;
            case 'P': frequency = NoteToFrequency(3, 9); break;
            case -37: frequency = NoteToFrequency(3, 10); break;

            // ASDF row
            case 'A': frequency = NoteToFrequency(2, 0); break;
            case 'S': frequency = NoteToFrequency(2, 1); break;
            case 'D': frequency = NoteToFrequency(2, 2); break;
            case 'F': frequency = NoteToFrequency(2, 3); break;
            case 'G': frequency = NoteToFrequency(2, 4); break;
            case 'H': frequency = NoteToFrequency(2, 5); break;
            case 'J': frequency = NoteToFrequency(2, 6); break;
            case 'K': frequency = NoteToFrequency(2, 7); break;
            case 'L': frequency = NoteToFrequency(2, 8); break;
            case -70: frequency = NoteToFrequency(2, 9); break;
            case -34: frequency = NoteToFrequency(2, 10); break;

            // ZXCV row
            case 'Z': frequency = NoteToFrequency(1, 0); break;
            case 'X': frequency = NoteToFrequency(1, 1); break;
            case 'C': frequency = NoteToFrequency(1, 2); break;
            case 'V': frequency = NoteToFrequency(1, 3); break;
            case 'B': frequency = NoteToFrequency(1, 4); break;
            case 'N': frequency = NoteToFrequency(1, 5); break;
            case 'M': frequency = NoteToFrequency(1, 6); break;
            case -68: frequency = NoteToFrequency(1, 7); break;
            case -66: frequency = NoteToFrequency(1, 8); break;
            case -65: frequency = NoteToFrequency(1, 9); break;
            case -95: frequency = NoteToFrequency(1, 10); break;  // right shift

            // left shift = low freq
            case 16: frequency = NoteToFrequency(0, 5); break;

            // left shift = low freq
            case -94: frequency = NoteToFrequency(0, 0); break;

            default: {
                return;
            }
        }

        // get a lock on our notes vector and add the new note
        std::lock_guard<std::mutex> guard(g_notesMutex);
        g_notes.push_back(SNote(frequency));
    }

    //--------------------------------------------------------------------------------------------------
    void OnEnterDemo () {
        g_rotateSound = false;
        g_pingPongDelay = false;
        g_cymbalsOn = false;
        g_voiceOn = false;
        printf("Letter keys to play notes.\r\nleft shift / control is super low frequency.\r\n");
        printf("1 = Toggle sound rotation\r\n");
        printf("2 = Toggle ping pong delay\r\n");
        printf("3 = Cymbals Sample\r\n");
        printf("4 = Voice Sample\r\n");
        printf("Interesting sound with both on = afqt also z zma z zmak, also shift,control repeated\r\n");

        // clear all the notes out
        std::lock_guard<std::mutex> guard(g_notesMutex);
        g_notes.clear();
    }
}
