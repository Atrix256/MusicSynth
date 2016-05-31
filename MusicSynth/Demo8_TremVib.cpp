//--------------------------------------------------------------------------------------------------
// Demo8_TremVib.cpp
//
// Logic for the demo of the same name
//
//--------------------------------------------------------------------------------------------------

#include "DemoMgr.h"
#include <algorithm>

namespace Demo8_TremVib {

    enum EWaveForm {
        e_waveSine,
        e_waveSaw,
        e_waveSquare,
        e_waveTriangle
    };

    enum EEffectSpeed {
        e_effectOff,
        e_effectSlow,
        e_effectMedium,
        e_effectFast
    };

    struct SNote {
        SNote(float frequency, EWaveForm waveForm, EEffectSpeed tremolo, EEffectSpeed vibrato)
            : m_frequency(frequency)
            , m_waveForm(waveForm)
            , m_tremolo(tremolo)
            , m_vibrato(vibrato)
            , m_age(0)
            , m_dead(false)
            , m_wantsKeyRelease(false)
            , m_releaseAge(0)
            , m_phase(0.0f) {}

        float           m_frequency;
        EWaveForm       m_waveForm;
        EEffectSpeed    m_tremolo;
        EEffectSpeed    m_vibrato;
        size_t          m_age;
        bool            m_dead;
        bool            m_wantsKeyRelease;
        size_t          m_releaseAge;
        float           m_phase;
    };

    std::vector<SNote>  g_notes;
    std::mutex          g_notesMutex;
    EWaveForm           g_currentWaveForm;
    EEffectSpeed        g_tremolo;
    EEffectSpeed        g_vibrato;

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
    inline float GetEffectFrequency (EEffectSpeed speed) {
        switch (speed) {
            case e_effectOff: return 0.0;
            case e_effectSlow: return 2.8f;
            case e_effectMedium: return 10.0f;
            case e_effectFast: return 20.0f;
        }

        return 0.0f;
    }

    //--------------------------------------------------------------------------------------------------
    inline float GenerateNoteSample (SNote& note, float sampleRate) {

        // calculate our age in seconds and advance our age in samples, by 1 sample
        float ageInSeconds = float(note.m_age) / sampleRate;
        ++note.m_age;

        // generate the envelope value for our note
        // decrease note volume a bit, because the volume adjustments don't seem to be quite enough
        float envelope = GenerateEnvelope(note, ageInSeconds, sampleRate) * 0.8f;

        // adjust our envelope by applying tremolo.
        // the tremolo affects the amplitude by multiplying it between 0.5 and 1.0 in a sine wave.
        envelope *= SineWave(ageInSeconds * GetEffectFrequency(note.m_tremolo)) * 0.25f + 0.5f;

        // calculate our frequency by starting with the base note and applying vibrato.
        // our vibratto adds plus or minus 5% of the frequency, on a sine wave.
        float frequency = note.m_frequency;
        frequency += frequency * SineWave(ageInSeconds * GetEffectFrequency(note.m_vibrato)) * 0.05f;

        // advance phase, making sure to keep it between 0 and 1
        note.m_phase += frequency / sampleRate;
        note.m_phase = std::fmodf(note.m_phase, 1.0);

        // generate the audio sample value for the current phase.
        switch (note.m_waveForm) {
            case e_waveSine:    return SineWave(note.m_phase) * envelope;
            case e_waveSaw:     return SawWaveBandLimited(note.m_phase, 10) * envelope;
            case e_waveSquare:  return SquareWaveBandLimited(note.m_phase, 10) * envelope;
            case e_waveTriangle:return TriangleWaveBandLimited(note.m_phase, 10) * envelope;
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
    void OnKey (char key, bool pressed) {

        // pressing numbers switches instruments, or adjusts effects
        if (pressed) {
            switch (key)
            {
                case '1': g_currentWaveForm = e_waveSine; printf("instrument: sine\r\n"); return;
                case '2': g_currentWaveForm = e_waveSaw; printf("instrument: bl saw\r\n"); return;
                case '3': g_currentWaveForm = e_waveSquare; printf("instrument: bl square\r\n"); return;
                case '4': g_currentWaveForm = e_waveTriangle; printf("instrument: bl triangle\r\n"); return;
                case '5': g_tremolo = (EEffectSpeed)(((int)g_tremolo + 1) % (e_effectFast+1)); printf("tremolo = %i\r\n", g_tremolo); return;
                case '6': g_vibrato = (EEffectSpeed)(((int)g_vibrato + 1) % (e_effectFast+1)); printf("vibrato = %i\r\n", g_vibrato); return;
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
        g_notes.push_back(SNote(frequency, g_currentWaveForm, g_tremolo, g_vibrato));
    }

    //--------------------------------------------------------------------------------------------------
    void OnEnterDemo () {
        g_currentWaveForm = e_waveSine;
        g_tremolo = e_effectOff;
        g_vibrato = e_effectOff;
        printf("Letter keys to play notes.\r\nleft shift / control is super low frequency.\r\n");
        printf("1 = Sine\r\n");
        printf("2 = Band Limited Saw\r\n");
        printf("3 = Band Limited Square\r\n");
        printf("4 = Band Limited Triangle\r\n");
        printf("5 = Cycle Tremolo\r\n");
        printf("6 = Cycle Vibrato\r\n");
    }
}
