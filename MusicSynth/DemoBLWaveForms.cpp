//--------------------------------------------------------------------------------------------------
// DemoBLWaveForms.cpp
//
// Logic for the demo of the same name
//
//--------------------------------------------------------------------------------------------------

#include "DemoMgr.h"
#include <algorithm>

namespace DemoBLWaveForms {

    enum EWaveForm {
        e_waveSine,
        e_waveSaw,
        e_waveSquare,
        e_waveTriangle
    };

    const char* WaveFormToString (EWaveForm waveForm) {
        switch (waveForm) {
            case e_waveSine: return "Sine";
            case e_waveSaw: return "Bandlimited Saw";
            case e_waveSquare: return "Bandlimited Square";
            case e_waveTriangle: return "Bandlimited Triangle";
        }
        return "???";
    }

    struct SNote {
        SNote(float frequency, EWaveForm waveForm)
            : m_frequency(frequency)
            , m_waveForm(waveForm)
            , m_age(0)
            , m_dead(false)
            , m_wantsKeyRelease(false)
            , m_releaseAge(0) {}

        float       m_frequency;
        EWaveForm   m_waveForm;
        size_t      m_age;
        bool        m_dead;
        bool        m_wantsKeyRelease;
        size_t      m_releaseAge;
    };

    std::vector<SNote>  g_notes;
    std::mutex          g_notesMutex;
    EWaveForm           g_currentWaveForm;

    //--------------------------------------------------------------------------------------------------
    void OnInit() { }

    //--------------------------------------------------------------------------------------------------
    void OnExit() { }

    //--------------------------------------------------------------------------------------------------
    inline float GenerateEnvelope (SNote& note, float ageInSeconds, float sampleRate) {

        // this just puts a short envelope on the beginning and end of the note and kills the note
        // when the release envelope is done.

        float envelope = 0.0f;

        static const float c_envelopeTime = 0.1f;

        // if the key isn't yet released
        if (note.m_releaseAge == 0) {
            // release the key if it wants to be released and has done the intro envelope
            if (note.m_wantsKeyRelease && ageInSeconds > c_envelopeTime) {
                note.m_releaseAge = note.m_age;
            }
            // else do the intro envelope
            else {
                envelope = Envelope2Pt(
                    ageInSeconds,
                    0.0f, 0.0f,
                    c_envelopeTime, 1.0f
                );
            }
        }

        // if the key has been released, apply the outro envelope
        if (note.m_releaseAge != 0) {

            float releaseAgeInSeconds = float(note.m_releaseAge) / sampleRate;

            float secondsInRelease = ageInSeconds - releaseAgeInSeconds;

            envelope = Envelope2Pt(
                secondsInRelease,
                0.0f, 1.0f,
                c_envelopeTime, 0.0f
            );

            // kill the note when the release is done
            if (secondsInRelease > c_envelopeTime)
                note.m_dead = true;
        }

        return envelope;
    }

    //--------------------------------------------------------------------------------------------------
    inline float GenerateNoteSample (SNote& note, float sampleRate) {

        // calculate our age in seconds and advance our age in samples, by 1 sample
        float ageInSeconds = float(note.m_age) / sampleRate;
        ++note.m_age;

        // generate the envelope value for our note
        // decrease note volume a bit, because the volume adjustments don't seem to be quite enough
        float envelope = GenerateEnvelope(note, ageInSeconds, sampleRate) * 0.8f;

        // generate the audio sample value for the current time.
        // Note that it is ok that we are basing audio samples on age instead of phase, because the
        // frequency never changes and we envelope the front and back to avoid popping.
        float phase = std::fmodf(ageInSeconds * note.m_frequency, 1.0f);
        switch (note.m_waveForm) {
            case e_waveSine:    return SineWave(phase) * envelope;
            case e_waveSaw:     return SawWaveBandLimited(phase, 10) * envelope;
            case e_waveSquare:  return SquareWaveBandLimited(phase, 10) * envelope;
            case e_waveTriangle:return TriangleWaveBandLimited(phase, 10) * envelope;
        }

        return 0.0f;
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
    void StopNote (float frequency) {

        // get a lock on our notes vector
        std::lock_guard<std::mutex> guard(g_notesMutex);

        // Any note that is this frequency should note that it wants to enter released state.
        std::for_each(
            g_notes.begin(),
            g_notes.end(),
            [frequency] (SNote& note) {
                if (note.m_frequency == frequency) {
                    note.m_wantsKeyRelease = true;
                }
            }
        );
    }

    //--------------------------------------------------------------------------------------------------
    void ReportParams() {
        printf("Instrument: %s\r\n", WaveFormToString(g_currentWaveForm));
    }

    //--------------------------------------------------------------------------------------------------
    void OnKey (char key, bool pressed) {

        // pressing numbers switches instruments
        if (pressed) {
            switch (key)
            {
                case '1': g_currentWaveForm = e_waveSine; ReportParams(); return;
                case '2': g_currentWaveForm = e_waveSaw; ReportParams(); return;
                case '3': g_currentWaveForm = e_waveSquare; ReportParams(); return;
                case '4': g_currentWaveForm = e_waveTriangle; ReportParams(); return;
            }
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


        // if releasing a note, we need to find and kill the flute note of the same frequency
        if (!pressed) {
            StopNote(frequency);
            return;
        }

        // get a lock on our notes vector and add the new note
        std::lock_guard<std::mutex> guard(g_notesMutex);
        g_notes.push_back(SNote(frequency, g_currentWaveForm));
    }

    //--------------------------------------------------------------------------------------------------
    void OnEnterDemo () {
        g_currentWaveForm = e_waveSine;
        printf("Letter keys to play notes.\r\nleft shift / control is super low frequency.\r\n");
        printf("1 = Sine\r\n");
        printf("2 = Band Limited Saw\r\n");
        printf("3 = Band Limited Square\r\n");
        printf("4 = Band Limited Triangle\r\n");
        printf("\r\nInstructions:\r\n");
        printf("Play diff instruments. Mention smoother sounds.\r\n");

        // clear all the notes out
        std::lock_guard<std::mutex> guard(g_notesMutex);
        g_notes.clear();
    }
}
