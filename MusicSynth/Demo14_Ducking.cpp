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

    enum ESample {
        e_drum1,
        e_drum2,
        e_drum3,
    };

    struct SNote {
        SNote(ESample sample)
            : m_sample(sample)
            , m_age(0)
            , m_dead(false) {}

        ESample m_sample;
        size_t  m_age;
        bool    m_dead;
    };

    std::vector<SNote>  g_notes;
    std::mutex          g_notesMutex;

    bool                g_musicOn;
    bool                g_duckingOn;
    bool                g_drumsOn;

    SWavFile            g_drumSamples[3];

    //--------------------------------------------------------------------------------------------------
    void OnInit() {
        if (!g_drumSamples[0].Load("Samples/clap.wav", CDemoMgr::GetNumChannels(), (size_t)CDemoMgr::GetSampleRate()))
            printf("Could not load Samples/clap.wav.\r\n");

        if (!g_drumSamples[1].Load("Samples/kick.wav", CDemoMgr::GetNumChannels(), (size_t)CDemoMgr::GetSampleRate()))
            printf("Could not load Samples/kick.wav.\r\n");

        if (!g_drumSamples[2].Load("Samples/ting.wav", CDemoMgr::GetNumChannels(), (size_t)CDemoMgr::GetSampleRate()))
            printf("Could not load Samples/ting.wav.\r\n");
    }

    //--------------------------------------------------------------------------------------------------
    void OnExit() { }

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
            
            // add up all samples to get the final value.
            float value = 0.0f;
            std::for_each(
                g_notes.begin(),
                g_notes.end(),
                [&value, numChannels](SNote& note) {
                    size_t sampleIndex = note.m_age*numChannels;
                    if (sampleIndex >= g_drumSamples[note.m_sample].m_numSamples) {
                        note.m_dead = true;
                        return;
                    }

                    value += g_drumSamples[note.m_sample].m_samples[sampleIndex];
                    ++note.m_age;
                }
            );

            // if we shouldn't hear the drums, silence them
            if (!g_drumsOn)
                value = 0.0f;

            // get our music sample, apply ducking if we should, and add it to our other sample
            float musicSample = GenerateMusicSample(sample, sampleRate);
            if (g_duckingOn)
                musicSample *= 1.0f;
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

        // figure out what sample to play
        ESample sample = e_drum1;
        float frequency = 0.0f;
        switch (key) {
            case 'Q': sample = e_drum1; break;
            case 'W': sample = e_drum2; break;
            case 'E': sample = e_drum3; break;
            default: {
                return;
            }
        }

        // get a lock on our notes vector and add the new note
        std::lock_guard<std::mutex> guard(g_notesMutex);
        g_notes.push_back(SNote(sample));
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

* update description
* make everything work again
* there is a lot of popping, may need to envelope it, or look into why.
 ? maybe resampling?

* I think we might need a different envelope for ducking music than for the sounds themselves.

* make the bg music not suck.  Maybe take it from lament of tim curry, or maybe play ghost jazz.
 * ghost jazz not sounding right, fix notes.
 * also sounds shitty.
 

* make demo 8 show frequency of vib / trem as you change it instead of just slow / fast / etc

* we are hard coding the channels and sample rate... no good!

*/