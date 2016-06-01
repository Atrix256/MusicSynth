//--------------------------------------------------------------------------------------------------
// Demo14_Ducking.cpp
//
// Logic for the demo of the same name
//
//--------------------------------------------------------------------------------------------------

#include "DemoMgr.h"
#include "AudioEffects.h"
#include <algorithm>

namespace Demo14_Ducking {

    enum EInstrument {
        e_drum,
        e_cymbal
    };

    struct SNote {
        SNote(float frequency, EInstrument instrument)
            : m_frequency(frequency)
            , m_instrument(instrument)
            , m_age(0)
            , m_dead(false)
            , m_releaseAge(0)
            , m_phase(0.0f) {}

        float       m_frequency;
        EInstrument m_instrument;
        size_t      m_age;
        bool        m_dead;
        size_t      m_releaseAge;
        float       m_phase;
    };

    std::vector<SNote>  g_notes;
    std::mutex          g_notesMutex;

    bool                g_musicOn;
    bool                g_duckingOn;
    bool                g_drumsOn;

    //--------------------------------------------------------------------------------------------------
    inline float GenerateEnvelope_Drum (SNote& note, float ageInSeconds) {
        // use an envelope that sounds "drum like"
        float envelope = Envelope4Pt(
            ageInSeconds,
            0.000f, 0.0f,
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
    inline float GenerateNoteSample_Cymbal (SNote& note, float sampleRate, float ageInSeconds, float& envelope) {
        
        // make the envelope
        envelope = GenerateEnvelope_Cymbal(note, ageInSeconds);

        // return noise shaped by the envelope, taken down in amplitude, so it isn't so loud.
        return Noise() * envelope * 0.25f;
    }

    //--------------------------------------------------------------------------------------------------
    inline float GenerateNoteSample (SNote& note, float sampleRate, float& envelope) {

        // calculate our age in seconds and advance our age in samples, by 1 sample
        float ageInSeconds = float(note.m_age) / sampleRate;
        ++note.m_age;

        if (note.m_instrument == e_cymbal)
            return GenerateNoteSample_Cymbal(note, sampleRate, ageInSeconds, envelope);

        // make a drum envelope
        envelope = GenerateEnvelope_Drum(note, ageInSeconds);

        // make frequency decay over time if we should
        float frequency = note.m_frequency;
        if (ageInSeconds > 0.020f) {
            float percent = (ageInSeconds - 0.020f) / 0.175f;
            frequency = Lerp(frequency, frequency*0.2f, percent);
        }

        // advance phase
        note.m_phase = std::fmodf(note.m_phase + frequency / sampleRate, 1.0f);

        // generate the sine value for the current time.
        return SineWave(note.m_phase) * envelope;
    }

    //--------------------------------------------------------------------------------------------------
    float GenerateMusicSample (size_t timeOffset, float sampleRate) {

        // handle turning on and off music, and getting the base line time when music is started
        static bool musicWasOn = false;
        static size_t musicStartSample = 0;
        bool musicIsOn = g_musicOn;
        static float phase = 0.0f;
        if (musicWasOn != musicIsOn) {
            musicWasOn = musicIsOn;
            musicStartSample = CDemoMgr::GetSampleClock();
            phase = 0.0f;
        }

        // don't generate notes if music is off
        if (!musicIsOn)
            return 0.0f;

        // get the current music time in samples and seconds
        size_t timeSamples = CDemoMgr::GetSampleClock() - musicStartSample + timeOffset;
        float timeSeconds = float(timeSamples) / sampleRate;

        // get the time in quarter seconds
        size_t quarterSeconds = size_t(timeSeconds*4.0f);
        float quarterSecondsPercent = std::fmodf(timeSeconds*4.0f, 1.0f);

        // get the frequency for the current time
        const float c_octave = 2.0f;
        const float c_baseNote = 6.0f;
        float frequency = 0.0f;
        quarterSeconds = quarterSeconds % 32;
        if (quarterSeconds < 16) {
            switch (quarterSeconds % 8) {
                case 0: frequency = NoteToFrequency(c_octave, c_baseNote + 0); break;
                case 1: frequency = NoteToFrequency(c_octave, c_baseNote + 2); break;
                case 2: frequency = NoteToFrequency(c_octave, c_baseNote + 4); break;
                case 3: frequency = 0.0f; break;
                case 4: frequency = NoteToFrequency(c_octave, c_baseNote + 0); break;
                case 5: frequency = NoteToFrequency(c_octave, c_baseNote + 4); break;
                case 6: frequency = 0.0f; break;
                case 7: frequency = 0.0f; break;
            }
        }
        else {
            switch (quarterSeconds % 16) {
                case  0: frequency = NoteToFrequency(c_octave, c_baseNote + 6); break;
                case  1: frequency = NoteToFrequency(c_octave, c_baseNote + 4); break;
                case  2: frequency = NoteToFrequency(c_octave, c_baseNote + 2); break;
                case  3: frequency = 0.0f; break;
                case  4: frequency = NoteToFrequency(c_octave, c_baseNote + 0); break;
                case  5: frequency = NoteToFrequency(c_octave, c_baseNote + 2); break;
                case  6: frequency = 0.0f; break;
                case  7: frequency = 0.0f; break;

                case  8: frequency = NoteToFrequency(c_octave, c_baseNote + 4); break;
                case  9: frequency = NoteToFrequency(c_octave, c_baseNote + 2); break;
                case 10: frequency = NoteToFrequency(c_octave, c_baseNote + 0); break;
                case 11: frequency = NoteToFrequency(c_octave, c_baseNote + 0); break;
                case 12: frequency = NoteToFrequency(c_octave, c_baseNote - 2); break;
                case 13: frequency = NoteToFrequency(c_octave, c_baseNote + 0); break;
                case 14: frequency = 0.0f; break;
                case 15: frequency = 0.0f; break;
            }
        }


        /*
        // get the frequency for the current time
        quarterSeconds = quarterSeconds % 4;
        float frequency1 = 0.0f;
        float frequency2 = 0.0f;
        switch (quarterSeconds) {
            case 0: frequency1 = NoteToFrequency(2, 0); frequency2 = NoteToFrequency(2, 1); break;
            case 1: frequency1 = NoteToFrequency(2, 1); frequency2 = NoteToFrequency(2, 3); break;
            case 2: frequency1 = NoteToFrequency(2, 3); frequency2 = NoteToFrequency(2, 2); break;
            case 3: frequency1 = NoteToFrequency(2, 2); frequency2 = NoteToFrequency(2, 0); break;
        }
        float frequency = Lerp(frequency1, frequency2, quarterSecondsPercent);
        */

        // make a quarter note envelope
        float noteEnvelope = Envelope4Pt(
            quarterSecondsPercent,
            0.0f, 0.0f,
            0.1f, 1.0f,
            0.9f, 1.0f,
            1.0f, 0.0f
        );

        // get the value
        float ret = SawWaveBandLimited(phase, 3) * 0.3f + SquareWaveBandLimited(phase, 5) * 0.7f;

        // advance phase
        phase = std::fmodf(phase + frequency / sampleRate, 1.0f);

        // return the value, with a starting envelope to get rid of the initial pop, and the note envelope
        return ret * Envelope2Pt(timeSeconds, 0.0f, 0.0f, 0.1f, 0.5f) * noteEnvelope;
    }

    //--------------------------------------------------------------------------------------------------
    void GenerateAudioSamples (float *outputBuffer, size_t framesPerBuffer, size_t numChannels, float sampleRate) {

        // get a lock on our notes vector
        std::lock_guard<std::mutex> guard(g_notesMutex);

        // for every sample in our output buffer
        for (size_t sample = 0; sample < framesPerBuffer; ++sample, outputBuffer += numChannels) {
            
            // add up all notes to get the final value.
            float value = 0.0f;
            float maxEnvelope = 0.0f;
            std::for_each(
                g_notes.begin(),
                g_notes.end(),
                [&value, sampleRate, &maxEnvelope](SNote& note) {
                    float envelope = 0.0f;
                    value += GenerateNoteSample(note, sampleRate, envelope);
                    if (envelope > maxEnvelope)
                        maxEnvelope = envelope;
                }
            );

            // if we shouldn't hear the drums, silence them
            if (!g_drumsOn)
                value = 0.0f;

            // get our music sample, apply ducking if we should, and add it to our other sample
            float musicSample = GenerateMusicSample(sample, sampleRate);
            if (g_duckingOn)
                musicSample *= (1.0f - maxEnvelope);
            value += musicSample;

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
    void ReportParams() {
        printf("Music: %s  Ducking: %s  Drums: %s\r\n", g_musicOn ? "On" : "Off", g_duckingOn ? "On" : "Off", g_drumsOn ? "On" : "Off");
    }

    //--------------------------------------------------------------------------------------------------
    void OnKey (char key, bool pressed) {

        // nothing to do on key release
        if (!pressed)
            return;

        // pressing 1 toggles music
        if (key == '1') {
            g_musicOn = !g_musicOn;
            ReportParams();
            return;
        }

        // pressing 2 toggles ducking
        if (key == '2') {
            g_duckingOn = !g_duckingOn;
            ReportParams();
            return;
        }

        // pressing 3 toggles drums
        if (key == '3') {
            g_drumsOn = !g_drumsOn;
            ReportParams();
            return;
        }

        // space bar = cymbals
        if (key == ' ') {
            std::lock_guard<std::mutex> guard(g_notesMutex);
            g_notes.push_back(SNote(0.0f, e_cymbal));
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
        g_notes.push_back(SNote(frequency, e_drum));
    }

    //--------------------------------------------------------------------------------------------------
    void OnEnterDemo () {
        g_musicOn = false;
        g_duckingOn = false;
        g_drumsOn = true;
        printf("Letter keys to play drums, space to play cymbals.\r\nleft shift / control is super low frequency.\r\n");
        printf("1 = toggle music\r\n");
        printf("2 = toggle ducking\r\n");
        printf("3 = toggle drums\r\n");
    }
}

/*

TODO:

* I think we might need a different envelope for ducking music than for the sounds themselves.

* make the bg music not suck.  Maybe take it from lament of tim curry, or maybe play ghost jazz.
 * ghost jazz not sounding right, fix notes.
 * also sounds shitty.
 

* make demo 8 show frequency of vib / trem as you change it instead of just slow / fast / etc

*/