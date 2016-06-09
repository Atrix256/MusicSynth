//--------------------------------------------------------------------------------------------------
// DemoFMSynth.cpp
//
// Logic for the demo of the same name
//
//--------------------------------------------------------------------------------------------------

#include "DemoMgr.h"
#include <algorithm>

namespace DemoFMSynth {


    // 300, 50 = metalic sounding organ thing
    // 3000, 50 = smoother sounding thing.

    enum EMode {
        e_modeNormal,
        e_modeSpeed1,
        e_modeSpeed2,
        e_modeSpeed3,
        e_modeDepth2,
        e_modeDepth3,

        e_modeFM1,
        e_modeFM2,
        e_modeFM3,

        e_modeCount
    };

    const char* ModeToString (EMode mode) {
        switch (mode) {
            case e_modeNormal: return "Normal";
            case e_modeSpeed1: return "Speed1 (10 hz, +- 10hz)";
            case e_modeSpeed2: return "Speed2 (100hz, +- 10hz)";
            case e_modeSpeed3: return "Speed3 (500hz, +- 10hz)";
            case e_modeDepth2: return "Depth2 (500hz, +-100hz)";
            case e_modeDepth3: return "Depth3 (500hz, +-500hz)";
            case e_modeFM1: return "FM1 - modulator 0.5, carrier. Horn.";
            case e_modeFM2: return "FM2 - modulator 0.1, modulator 2.5, carrier. Alien fx.";
            case e_modeFM3: return "FN3 - modulator 2.37, carrier. Diff envelope for modulator and carrier. bell and biased inverse bell. Metal Drum.";
        }
        return "???";
    }

    EMode g_mode = e_modeNormal;

    struct SNote {
        SNote(float frequency, EMode mode)
            : m_frequency(frequency)
            , m_mode(mode)
            , m_age(0)
            , m_dead(false)
            , m_wantsKeyRelease(false)
            , m_releaseAge(0)
            , m_phase(0.0f)
            , m_phase2(0.0f)
            , m_phase3(0.0f) {}

        float           m_frequency;
        EMode           m_mode;
        size_t          m_age;
        bool            m_dead;
        bool            m_wantsKeyRelease;
        size_t          m_releaseAge;
        float           m_phase;
        float           m_phase2;
        float           m_phase3;
    };

    std::vector<SNote>  g_notes;
    std::mutex          g_notesMutex;

    //--------------------------------------------------------------------------------------------------
    void OnInit() { }

    //--------------------------------------------------------------------------------------------------
    void OnExit() { }

    //--------------------------------------------------------------------------------------------------
    inline float GenerateEnvelope_Simple (SNote& note, float ageInSeconds, float sampleRate) {

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
    inline float GenerateEnvelope_Bell (SNote& note, float ageInSeconds) {
        // note lifetime
        static const float c_noteLifeTime = 1.00f;

        // use an envelope that sounds "bell like"
        float envelope = Envelope3Pt(
            ageInSeconds,
            0.0f , 0.0f,
            0.003f, 1.0f,
            c_noteLifeTime, 0.0f
        );

        // kill notes that are too old
        if (ageInSeconds > c_noteLifeTime)
            note.m_dead = true;

        return envelope;
    }

    //--------------------------------------------------------------------------------------------------
    inline float AdvanceSineWave (float& phase, float frequency, float sampleRate) {

        // calculate the sine wave value
        float ret = SineWave(phase);

        // advance phase
        phase = std::fmodf(phase + frequency / sampleRate, 1.0f);

        // return the sine wave value
        return ret;
    }

    //--------------------------------------------------------------------------------------------------
    inline float FMOperator (float& phase, float frequency, float modulationSample, float envelopeSample, float sampleRate) {
        return AdvanceSineWave(phase, frequency + modulationSample, sampleRate) * envelopeSample;
    }

    //--------------------------------------------------------------------------------------------------
    inline float GenerateNoteSample (SNote& note, float sampleRate) {

        // calculate our age in seconds and advance our age in samples, by 1 sample
        float ageInSeconds = float(note.m_age) / sampleRate;
        ++note.m_age;
        
        // calculate frequency for specific mode
        float frequency = note.m_frequency;
        switch (note.m_mode) {
            case e_modeNormal: {
                float envelope = GenerateEnvelope_Simple(note, ageInSeconds, sampleRate);

                return AdvanceSineWave(note.m_phase, note.m_frequency, sampleRate) * envelope;
            }
            case e_modeSpeed1: {
                float envelope = GenerateEnvelope_Simple(note, ageInSeconds, sampleRate);

                float modulatorWave = AdvanceSineWave(note.m_phase2, 10.0f, sampleRate) * 10.0f;
                return AdvanceSineWave(note.m_phase, note.m_frequency + modulatorWave, sampleRate) * envelope;
            }
            case e_modeSpeed2: {
                float envelope = GenerateEnvelope_Simple(note, ageInSeconds, sampleRate);

                float modulatorWave = AdvanceSineWave(note.m_phase2, 100.0f, sampleRate) * 10.0f;
                return AdvanceSineWave(note.m_phase, note.m_frequency + modulatorWave, sampleRate) * envelope;
            }
            case e_modeSpeed3: {
                float envelope = GenerateEnvelope_Simple(note, ageInSeconds, sampleRate);

                float modulatorWave = AdvanceSineWave(note.m_phase2, 500.0f, sampleRate) * 10.0f;
                return AdvanceSineWave(note.m_phase, note.m_frequency + modulatorWave, sampleRate) * envelope;
            }
            case e_modeDepth2: {
                float envelope = GenerateEnvelope_Simple(note, ageInSeconds, sampleRate);

                float modulatorWave = AdvanceSineWave(note.m_phase2, 500.0f, sampleRate) * 100.0f;
                return AdvanceSineWave(note.m_phase, note.m_frequency + modulatorWave, sampleRate) * envelope;
            }
            case e_modeDepth3: {
                float envelope = GenerateEnvelope_Simple(note, ageInSeconds, sampleRate);

                float modulatorWave = AdvanceSineWave(note.m_phase2, 500.0f, sampleRate) * 500.0f;
                return AdvanceSineWave(note.m_phase, note.m_frequency + modulatorWave, sampleRate) * envelope;
            }
            case e_modeFM1: {
                float envelope = GenerateEnvelope_Simple(note, ageInSeconds, sampleRate);

                float modulatorWave = AdvanceSineWave(note.m_phase2, note.m_frequency * 0.5f, sampleRate) * note.m_frequency;
                return AdvanceSineWave(note.m_phase, note.m_frequency + modulatorWave, sampleRate) * envelope;

                // the above is the same as this:
                //float modulatorWave = FMOperator(note.m_phase2, note.m_frequency * 0.5f, 0.0f, note.m_frequency, sampleRate);
                //return FMOperator(note.m_phase, note.m_frequency, modulatorWave, envelope, sampleRate);
            }
            case e_modeFM2: {
                float envelope = GenerateEnvelope_Simple(note, ageInSeconds, sampleRate);

                float modulatorWave2 = FMOperator(note.m_phase3, note.m_frequency * 0.1f, 0.0f, note.m_frequency, sampleRate);
                float modulatorWave = FMOperator(note.m_phase2, note.m_frequency * 2.5f, modulatorWave2, note.m_frequency, sampleRate);
                return FMOperator(note.m_phase, note.m_frequency, modulatorWave, envelope, sampleRate);
            }
            case e_modeFM3: {
                float envelope = GenerateEnvelope_Bell(note, ageInSeconds);
                float modEnvelope = Bias(1.0f - envelope, 0.9f);

                float modulatorWave = FMOperator(note.m_phase2, note.m_frequency * 2.37f, 0.0f, note.m_frequency * modEnvelope, sampleRate);
                return FMOperator(note.m_phase, note.m_frequency, modulatorWave, envelope, sampleRate);
            }
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

        if (pressed) {
            if (key == '1') {
                if (g_mode < e_modeCount - 1)
                    g_mode = EMode(g_mode + 1);
                printf("mode = %i %s\r\n", g_mode, ModeToString(g_mode));
                return;
            }
            else if (key == '2') {
                if (g_mode > 0)
                    g_mode = EMode(g_mode - 1);
                printf("mode = %i %s\r\n", g_mode, ModeToString(g_mode));
                return;
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
        g_notes.push_back(SNote(frequency, g_mode));
    }

    //--------------------------------------------------------------------------------------------------
    void OnEnterDemo () {
        g_mode = e_modeNormal;
        printf("Letter keys to play notes.\r\nleft shift / control is super low frequency.\r\n");
        printf("1 = Increase Mode\r\n");
        printf("2 = Decrease Mode\r\n");
        printf("\r\nInstructions:\r\n");
        printf("Go through the options talking about each.\r\n");

        // clear all the notes out
        std::lock_guard<std::mutex> guard(g_notesMutex);
        g_notes.clear();
    }
}
