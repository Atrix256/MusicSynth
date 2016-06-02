//--------------------------------------------------------------------------------------------------
// Demo5_Envelopes.cpp
//
// Logic for the demo of the same name
//
//--------------------------------------------------------------------------------------------------

#include "DemoMgr.h"
#include <algorithm>

namespace Demo5_Envelopes {

    enum EEnvelope {
        e_envelopeMinimal,
        e_envelopeBell,
        e_envelopeReverseBell,
        e_envelopeFlute,
    };

    const char* EnvelopeToString (EEnvelope envelope) {
        switch (envelope) {
            case e_envelopeMinimal: return "Sine";
            case e_envelopeBell: return "Bell";
            case e_envelopeReverseBell: return "Reverse Bell";
            case e_envelopeFlute: return "Flute";
        }
        return "???";
    }

    struct SNote {
        SNote(float frequency, EEnvelope envelope)
            : m_frequency(frequency)
            , m_envelope(envelope)
            , m_age(0)
            , m_dead(false)
            , m_wantsKeyRelease(false)
            , m_releaseAge(0) {}

        float       m_frequency;
        EEnvelope   m_envelope;
        size_t      m_age;
        bool        m_dead;
        bool        m_wantsKeyRelease;
        size_t      m_releaseAge;
    };

    std::vector<SNote>  g_notes;
    std::mutex          g_notesMutex;
    EEnvelope           g_currentEnvelope;

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
    inline float GenerateEnvelope_ReverseBell (SNote& note, float ageInSeconds) {
        // note lifetime
        static const float c_noteLifeTime = 1.00f;

        // use an envelope that sounds "bell like" but reversed in time
        float envelope = Envelope3Pt(
            ageInSeconds,
            0.0f, 0.0f,
            c_noteLifeTime-0.003f, 1.0f,
            c_noteLifeTime, 0.0f
        );

        // kill notes that are too old
        if (ageInSeconds > c_noteLifeTime)
            note.m_dead = true;

        return envelope;
    }

    //--------------------------------------------------------------------------------------------------
    inline float GenerateEnvelope_Flute (SNote& note, float ageInSeconds, float sampleRate) {
        
        /*
            We do an ADSR envelope for flute:  Attack, Decay, Sustain, Release.

            When the note is pressed, it always does an attack and decay envelope.  Attack takes it from
            0 volume to full volume, then Decay takes it down to the decay volume level.

            Then, as long as the key is held down, it will play at the decay volume.

            When the key is released, it will then do the decay envelope back to silence and then kill
            the note.

              A
              /\ D     S
             /  --------\
            /            \
           0              R (0)

        */
        

        // length of envelope sections
        static const float c_attackTime = 0.1f;
        static const float c_decayTime = 0.05f;
        static const float c_releaseTime = 0.1f;
        static const float c_noteInitialTime = c_attackTime + c_decayTime;

        // envelope volumes
        static const float c_attackVolume = 1.0f;
        static const float c_decayVolume = 0.4f;

        float envelope = 0.0f;

        // if the key isn't yet released
        if (note.m_releaseAge == 0) {
            // release the key if it wants to be released and has done the attack and decay
            if (note.m_wantsKeyRelease && ageInSeconds > c_noteInitialTime) {
                note.m_releaseAge = note.m_age;
            }
            // else do the attack and decay envelope
            else {
                envelope = Envelope3Pt(
                    ageInSeconds,
                    0.0f, 0.0f,
                    c_attackTime, c_attackVolume,
                    c_noteInitialTime, c_decayVolume
                );
            }
        }

        // if the key has been released, apply the release 
        if (note.m_releaseAge != 0) {

            float releaseAgeInSeconds = float(note.m_releaseAge) / sampleRate;

            float secondsInRelease = ageInSeconds - releaseAgeInSeconds;

            envelope = Envelope2Pt(
                secondsInRelease,
                0.0f, c_decayVolume,
                c_releaseTime, 0.0f
            );

            // kill the note when the release is done
            if (secondsInRelease > c_releaseTime)
                note.m_dead = true;
        }

        return envelope;
    }

    //--------------------------------------------------------------------------------------------------
    inline float GenerateNoteSample (SNote& note, float sampleRate) {

        float envelope = 0.0f;

        // calculate our age in seconds and advance our age in samples, by 1 sample
        float ageInSeconds = float(note.m_age) / sampleRate;
        ++note.m_age;

        // do the envelope specific behavior
        switch (note.m_envelope) {
            case e_envelopeMinimal:     envelope = GenerateEnvelope_Minimal(note, ageInSeconds); break;
            case e_envelopeBell:        envelope = GenerateEnvelope_Bell(note, ageInSeconds); break;
            case e_envelopeReverseBell: envelope = GenerateEnvelope_ReverseBell(note, ageInSeconds); break;
            case e_envelopeFlute:       envelope = GenerateEnvelope_Flute(note, ageInSeconds, sampleRate); break;
        }

        // generate the sine value for the current time.
        // Note that it is ok that we are basing audio samples on age instead of phase, because the
        // frequency never changes and we envelope the front and back to avoid popping.
        return  std::sinf(ageInSeconds*note.m_frequency*2.0f*c_pi) * envelope;
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
    void StopFluteNote (float frequency) {

        // get a lock on our notes vector
        std::lock_guard<std::mutex> guard(g_notesMutex);

        // Any note that is a flute note of this frequency should note that it wants to enter released
        // state.
        std::for_each(
            g_notes.begin(),
            g_notes.end(),
            [frequency] (SNote& note) {
                if (note.m_envelope == e_envelopeFlute && note.m_frequency == frequency) {
                    note.m_wantsKeyRelease = true;
                }
            }
        );
    }

    //--------------------------------------------------------------------------------------------------
    void ReportParams() {
        printf("Envelope: %s\r\n", EnvelopeToString(g_currentEnvelope));
    }

    //--------------------------------------------------------------------------------------------------
    void OnKey (char key, bool pressed) {

        // pressing numbers switch envelopes
        if (pressed) {
            switch (key)
            {
                case '1': g_currentEnvelope = e_envelopeMinimal; ReportParams(); return;
                case '2': g_currentEnvelope = e_envelopeBell; ReportParams(); return;
                case '3': g_currentEnvelope = e_envelopeReverseBell; ReportParams(); return;
                case '4': g_currentEnvelope = e_envelopeFlute; ReportParams(); return;
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


        // if releasing a note, we want to do nothing in most modes.
        // in flute mode, we need to find and kill the flute note of the same frequency
        if (!pressed) {
            if (g_currentEnvelope == e_envelopeFlute) {
                StopFluteNote(frequency);
            }
            return;
        }

        // get a lock on our notes vector and add the new note
        std::lock_guard<std::mutex> guard(g_notesMutex);
        g_notes.push_back(SNote(frequency, g_currentEnvelope));
    }

    //--------------------------------------------------------------------------------------------------
    void OnEnterDemo () {
        g_currentEnvelope = e_envelopeMinimal;
        printf("Letter keys to play notes.\r\nleft shift / control is super low frequency.\r\n");
        printf("1 = minimal\r\n");
        printf("2 = bell\r\n");
        printf("3 = reverse bell\r\n");
        printf("4 = flute\r\n");
    }
}
