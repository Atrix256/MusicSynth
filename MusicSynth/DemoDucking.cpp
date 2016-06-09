//--------------------------------------------------------------------------------------------------
// DemoDucking.cpp
//
// Logic for the demo of the same name
//
//--------------------------------------------------------------------------------------------------

#include "DemoMgr.h"
#include "AudioEffects.h"
#include "Samples.h"
#include <algorithm>

namespace DemoDucking {

    enum ESample {
        e_drum1,
        e_drum2,
        e_drum3,

        e_music
    };

    struct SNote {
        SNote(ESample sample, bool duck, bool muteSample)
            : m_sample(sample)
            , m_age(0)
            , m_dead(false)
            , m_duck(duck)
            , m_muteSample(muteSample) {}

        ESample m_sample;
        size_t  m_age;
        bool    m_dead;
        bool    m_duck;
        bool    m_muteSample;
    };

    std::vector<SNote>  g_notes;
    std::mutex          g_notesMutex;

    bool                g_musicOn;

    //--------------------------------------------------------------------------------------------------
    void OnInit() { }

    //--------------------------------------------------------------------------------------------------
    void OnExit() { }

    //--------------------------------------------------------------------------------------------------
    SWavFile& GetWavFile(ESample sample) {
        switch (sample) {
            case e_drum1:   return g_sample_clap;
            case e_drum2:   return g_sample_kick;
            case e_music:   return g_sample_pvd;
            default:        return g_sample_ting;
        }
    }

    //--------------------------------------------------------------------------------------------------
    float GenerateMusicSample (size_t sample, float sampleRate) {
        SWavFile& music = GetWavFile(e_music);

        // make the song loop by using modulus on the sample we were asked for
        size_t numSamples = (music.m_numSamples / music.m_numChannels);
        sample = sample % numSamples;

        // calculate and apply an envelope to the start and end of the sound
        const float c_envelopeTime = 0.005f;
        float ageInSeconds = float(sample) / float(sampleRate);
        float envelope = Envelope4Pt(
            ageInSeconds,
            0.0f, 0.0f,
            c_envelopeTime, 1.0f,
            music.m_lengthSeconds - c_envelopeTime, 1.0f,
            music.m_lengthSeconds, 0.0f
        );

        // return the current sample, multiplied by the envelope
        size_t sampleIndex = sample*music.m_numChannels;
        return music.m_samples[sampleIndex] * envelope;
    }

    //--------------------------------------------------------------------------------------------------
    void GenerateAudioSamples (float *outputBuffer, size_t framesPerBuffer, size_t numChannels, float sampleRate) {

        // handle starting or stopping music
        static bool musicWasOn = false;
        static size_t musicStarted = 0;
        bool musicIsOn = g_musicOn;
        if (musicIsOn != musicWasOn) {
            musicStarted = CDemoMgr::GetSampleClock();
            musicWasOn = musicIsOn;
        }

        // get a lock on our notes vector
        std::lock_guard<std::mutex> guard(g_notesMutex);

        // for every sample in our output buffer
        for (size_t sample = 0; sample < framesPerBuffer; ++sample, outputBuffer += numChannels) {
            
            // add up all samples to get the final value.
            float value = 0.0f;
            float duckingEnvelopeMax = 0.0f;
            std::for_each(
                g_notes.begin(),
                g_notes.end(),
                [&value, &duckingEnvelopeMax, numChannels, sampleRate](SNote& note) {

                    SWavFile& wavFile = GetWavFile(note.m_sample);

                    size_t sampleIndex = note.m_age*numChannels;
                    if (sampleIndex >= wavFile.m_numSamples) {
                        note.m_dead = true;
                        return;
                    }

                    // calculate and apply an envelope to the sound samples
                    float ageInSeconds = float(note.m_age) / float(sampleRate);
                    float envelope = Envelope4Pt(
                        ageInSeconds,
                        0.0f, 0.0f,
                        0.1f, 1.0f,
                        wavFile.m_lengthSeconds - 0.1f, 1.0f,
                        wavFile.m_lengthSeconds, 0.0f
                    );

                    float duckingEnvelope = Envelope4Pt(
                        ageInSeconds,
                        0.00f, 0.0f,
                        0.10f, 1.0f,
                        0.15f, 1.0f,
                        0.20f, 0.0f
                    );

                    if (note.m_duck && duckingEnvelope > duckingEnvelopeMax)
                        duckingEnvelopeMax = duckingEnvelope;

                    if (!note.m_muteSample)
                        value += wavFile.m_samples[sampleIndex] * envelope;
                    ++note.m_age;
                }
            );

            // don't completely duck the background music, so decrease our ducking envelope a bit
            duckingEnvelopeMax *= dBToAmplitude(-3.0f);

            // get our music sample, apply ducking, and add it to our other sample
            float duckingEnvelope = (1.0f - duckingEnvelopeMax);
            if (musicIsOn) {
                float musicSample = GenerateMusicSample(CDemoMgr::GetSampleClock() + sample - musicStarted, sampleRate);
                musicSample *= duckingEnvelope;
                value += musicSample;
            }

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
        printf("Music: %s\r\n", g_musicOn ? "On" : "Off");
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

        // figure out what sample to play
        ESample sample = e_drum1;
        bool duck = false;
        bool muteSample = false;
        float frequency = 0.0f;
        switch (key) {
            case 'Q': sample = e_drum1; break;
            case 'W': sample = e_drum2; break;
            case 'E': sample = e_drum3; break;
            
            case 'A': sample = e_drum1; duck = false; break;
            case 'S': sample = e_drum2; duck = true; break;
            case 'D': sample = e_drum3; duck = false; break;
            
            case 'Z': sample = e_drum1; duck = false; muteSample = true; break;
            case 'X': sample = e_drum2; duck = true; muteSample = true; break;
            case 'C': sample = e_drum3; duck = false; muteSample = true; break;
            default: {
                return;
            }
        }

        // get a lock on our notes vector and add the new note
        std::lock_guard<std::mutex> guard(g_notesMutex);
        g_notes.push_back(SNote(sample, duck, muteSample));
    }

    //--------------------------------------------------------------------------------------------------
    void OnEnterDemo () {
        g_musicOn = false;

        printf("1 = toggle music\r\n");
        printf("QWE = drum samples\r\n");
        printf("ASD = drum samples with ducking\r\n");
        printf("ZXC = ducking only\r\n");
        printf("\r\nInstructions:\r\n");
        printf("show the drum samples, then show with music on, highlight the ducking.\r\n");

        // clear all the notes out
        std::lock_guard<std::mutex> guard(g_notesMutex);
        g_notes.clear();
    }
}
