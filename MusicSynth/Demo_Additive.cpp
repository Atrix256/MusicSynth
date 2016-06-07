//--------------------------------------------------------------------------------------------------
// Demo_Additive.cpp
//
// Logic for the demo of the same name
//
//--------------------------------------------------------------------------------------------------

#include "DemoMgr.h"
#include "AudioEffects.h"
#include <algorithm>

namespace DemoAdditive {

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

    //--------------------------------------------------------------------------------------------------
    void OnInit() { }

    //--------------------------------------------------------------------------------------------------
    void OnExit() { }

    //--------------------------------------------------------------------------------------------------
    inline float GenerateEnvelope_Minimal (SNote& note, float ageInSeconds) {
        // note lifetime
        static const float c_noteLifeTime = 0.25f;

        // envelope point calculations
        static const float c_noteEnvelope = 0.05f;
        static const float c_envelopePtA = 0.0f;
        static const float c_envelopePtB = c_noteEnvelope;
        static const float c_envelopePtC = c_noteLifeTime - c_noteEnvelope;
        static const float c_envelopePtD = c_noteLifeTime;

        // put a small envelope on the front and back
        float envelope = Envelope4Pt(
            ageInSeconds,
            c_envelopePtA, 0.0f,
            c_envelopePtB, 1.0f,
            c_envelopePtC, 1.0f,
            c_envelopePtD, 0.0f
        );

        // kill notes that are too old
        if (ageInSeconds > c_noteLifeTime)
            note.m_dead = true;

        return envelope;
    }

    //--------------------------------------------------------------------------------------------------
    inline float GenerateEnvelope_Drum (SNote& note, float ageInSeconds, bool pop) {
        // use an envelope that sounds "drum like"
        float envelope = Envelope4Pt(
            ageInSeconds,
            0.000f, pop ? 1.0f : 0.0f,  // pop by starting at full volume, if we should
            0.010f, 1.0f,            //  10ms: attack (silence -> full volume)
            0.020f, 1.0f,            //  10ms: hold (full volume)
            0.195f, 0.0f             // 175ms: decay (full volume -> silence)
        );

        // kill notes that are too old
        if (ageInSeconds > 0.195)
            note.m_dead = true;

        return envelope;
    }

    //--------------------------------------------------------------------------------------------------
    inline float GenerateEnvelope_Cymbal (SNote& note, float ageInSeconds) {
        // use an envelope that sounds "drum like"
        float envelope = Envelope5Pt(
            ageInSeconds,
            0.000f, 0.0f,
            0.010f, 1.0f,            //  10ms: attack (silence -> full volume)
            0.020f, 1.0f,            //  10ms: hold (full volume)
            0.040f, 0.2f,            //  20ms: decay1 (full volume -> less volume)
            0.215f, 0.0f             // 175ms: decay (less volume -> silence)
        );

        // kill notes that are too old
        if (ageInSeconds > 0.215)
            note.m_dead = true;

        return envelope;
    }

    //--------------------------------------------------------------------------------------------------
    inline float GenerateNoteSample (SNote& note, float sampleRate) {

        float c_decayTime = 1.5f;
        size_t c_numHarmonics = 5;

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
            float envelope = Envelope3Pt(
                ageInSeconds,
                0.0f, 0.0f,
                0.01f + 0.01f * float(index), 1.0f,
                c_decayTime / float(index), 0.0f
            );
            float phase = std::fmodf(note.m_phase * float(index) , 1.0f);
            ret += SquareWaveBandLimited(phase, 5) * envelope;
        }

        // advance phase
        note.m_phase = std::fmodf(note.m_phase + note.m_frequency / sampleRate, 1.0f);

        // return the value
        return ret;
    }

    //--------------------------------------------------------------------------------------------------
    void GenerateAudioSamples (float *outputBuffer, size_t framesPerBuffer, size_t numChannels, float sampleRate) {

        // get a lock on our notes vector
        std::lock_guard<std::mutex> guard(g_notesMutex);

        // for every sample in our output buffer
        for (size_t sample = 0; sample < framesPerBuffer; ++sample, outputBuffer += numChannels) {
            
            // add up all notes to get the final value.
            float value = 0.0f;
            std::for_each(
                g_notes.begin(),
                g_notes.end(),
                [&value, sampleRate](SNote& note) {
                    value += GenerateNoteSample(note, sampleRate);
                }
            );

            // copy the value to all audio channels
            for (size_t channel = 0; channel < numChannels; ++channel)
                outputBuffer[channel] = value;
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
    void OnKey (char key, bool pressed) {

        // nothing to do on key release
        if (!pressed)
            return;

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
        printf("Letter keys to play drums.\r\nleft shift / control is super low frequency.\r\n");

        // clear all the notes out
        std::lock_guard<std::mutex> guard(g_notesMutex);
        g_notes.clear();
    }
}
