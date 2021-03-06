//--------------------------------------------------------------------------------------------------
// DemoMgr.h
//
// Handles switching between demos, passing the demos key events, knowing when to exit the app etc.
//
//--------------------------------------------------------------------------------------------------

#pragma once

#include <stdio.h>
#include "AudioUtils.h"
#include "WavFile.h"
#include <vector>
#include <mutex>
#include <queue>
#include <memory>
#include "Samples.h"

//--------------------------------------------------------------------------------------------------
enum EDemo {
    e_demoUnknown = -1,
#define DEMO(name) e_demo##name,
#include "DemoList.h"
    e_demoCount,
    e_demoLast = e_demoCount - 1,
    e_demoFirst = e_demoUnknown + 1
};

//--------------------------------------------------------------------------------------------------
// forward declarations of demo specific functions, in their respective namespaces
#define DEMO(name)  namespace Demo##name {\
    void GenerateAudioSamples (float *outputBuffer, size_t framesPerBuffer, size_t numChannels, float sampleRate); \
    void OnKey (char key, bool pressed); \
    void OnEnterDemo (); \
    void OnInit (); \
    void OnExit (); \
};
#include "DemoList.h"

//--------------------------------------------------------------------------------------------------
class CDemoMgr {
public:
    inline static void Init (float sampleRate, size_t numChannels) {
        printf("\r\n\r\n\r\n\r\n============================================\r\n");
        printf("Welcome!\r\nUp and down to adjust volume.\r\nLeft and right to change demo.\r\nEnter to toggle clipping.\r\nbackspace to toggle audio recording.\r\nEscape to exit.\r\n");
        printf("sampleRate = %0.0f, numChannels = %i\r\n", sampleRate, numChannels);
        printf("============================================\r\n\r\n");

        s_sampleRate = sampleRate;
        s_numChannels = numChannels;

        // load the audio samples
        LoadSamples();

        // give each demo and OnInit() call
        #define DEMO(name) Demo##name::OnInit();
        #include "DemoList.h"

        // let the demo know we've entered too
        OnEnterDemo();
    }

    inline static void OnEnterDemo () {

        printf("\r\n\r\n--------------------------------------------\r\n");

        // pass this call onto the current demo
        switch (s_currentDemo) {
            #define DEMO(name) case e_demo##name: printf( "Demo %i/%i: " #name "\r\n", e_demo##name+1, e_demoCount); Demo##name::OnEnterDemo(); break;
            #include "DemoList.h"
        }

        printf("--------------------------------------------\r\n\r\n");
    }

    inline static void GenerateAudioSamples (float *outputBuffer, size_t framesPerBuffer, size_t numChannels, float sampleRate) {
        // pass this call onto the current demo
        switch (s_currentDemo) {
            #define DEMO(name) case e_demo##name: Demo##name::GenerateAudioSamples(outputBuffer, framesPerBuffer, numChannels, sampleRate); break;
            #include "DemoList.h"
        }

        // store off the original value of outputBuffer so we can use it for recording
        float *bufferStart = outputBuffer;

        // apply volume adjustment smoothly over the buffer window via a lerp of amplitude.
        // also apply clipping.
        bool clip = s_clippingOn;
        static float lastVolumeMultiplier = 1.0;
        float volumeMultiplier = dBToAmplitude((1.0f - float(s_volumeMultiplier)/20.0f) * -60.0f);
        for (size_t sample = 0; sample < framesPerBuffer; ++sample, outputBuffer += numChannels) {
            // lerp the volume change across the buffer
            float percent = float(sample) / float(framesPerBuffer);
            float volume = Lerp(lastVolumeMultiplier, volumeMultiplier, percent);

            // apply volume and clipping
            for (size_t channel = 0; channel < numChannels; ++channel) {
                float value = outputBuffer[channel] * volume;

                if (clip) {
                    if (value > 1.0f)
                        value = 1.0f;
                    else if (value < -1.0f)
                        value = -1.0f;
                }
                outputBuffer[channel] = value;
            }
        }

        s_sampleClock += framesPerBuffer;

        lastVolumeMultiplier = volumeMultiplier;

        // if we are recording, add this frame to our frame queue
        if (IsRecording())
            AddRecordingBuffer(bufferStart, framesPerBuffer, numChannels, sampleRate);
    }

    static void OnKey(char key, bool pressed) {
        switch (key) {
            // exit when escape is pressed on any demo
            case 27: Exit(); return;
            // enter toggles clipping
            case 13: {
                if (pressed) {
                    s_clippingOn = !s_clippingOn;
                    printf("clipping = %s\r\n", s_clippingOn ? "on" : "off");
                }
                return;
            }
            // backspace toggles recording
            case 8: {
                if (pressed) {
                    if (!s_recordingWavFile)
                        StartRecording();
                    else
                        StopRecording();
                }
                return;
            }
            // when up arrow pressed, increase volume
            case 38: {
                if (pressed) {
                    s_volumeMultiplier++;
                    if (s_volumeMultiplier > 20)
                        s_volumeMultiplier = 20;
                    printf("Master Volume = %i%%\r\n", s_volumeMultiplier * 5);
                }
                return;
            }
            // when down arrow pressed, decrease volume
            case 40: {
                if (pressed) {
                    s_volumeMultiplier--;
                    if (s_volumeMultiplier < 0)
                        s_volumeMultiplier = 0;
                    printf("Master Volume = %i%%\r\n", s_volumeMultiplier * 5);
                }
                return;
            }
            // left arrow means go to previous demo
            case 37: {
                if (pressed && s_currentDemo > e_demoFirst) {
                    s_currentDemo = EDemo(int(s_currentDemo) - 1);
                    OnEnterDemo();
                }
                return;
            }
            // right arrow means go to next demo
            case 39: {
                if (pressed && s_currentDemo < e_demoLast) {
                    s_currentDemo = EDemo(int(s_currentDemo) + 1);
                    OnEnterDemo();
                }
                return;
            }
        }

        // else pass this key event onto the current demo
        switch (s_currentDemo) {
            #define DEMO(name) case e_demo##name: Demo##name::OnKey(key, pressed); break;
            #include "DemoList.h"
        }
    }

    static bool IsRecording() { return s_recordingWavFile != nullptr; }

    static void StartRecording ();
    static void StopRecording ();
    static void Update ();

    static void Exit () {
        if (IsRecording())
            StopRecording();
        s_exit = true;

        // tell all of our demos about exit in case they need to do any clean up
        #define DEMO(name) Demo##name::OnExit();
        #include "DemoList.h"
    }

    static bool WantsExit () { return s_exit; }

    static size_t GetSampleClock () { return s_sampleClock; }
    static size_t GetNumChannels () { return s_numChannels; }
    static float GetSampleRate () { return s_sampleRate; }

private:
    static void FlushRecordingBuffers ();
    static void ClearRecordingBuffers ();
    static void AddRecordingBuffer (float *buffer, size_t framesPerBuffer, size_t numChannels, float sampleRate);

private:
    struct SRecordingBuffer {

        SRecordingBuffer() {
            m_buffer = nullptr;
            m_framesPerBuffer = 0;
            m_numChannels = 0;
            m_sampleRate = 0.0f;
        }

        ~SRecordingBuffer() {
            delete[] m_buffer;
            m_buffer = nullptr;
            m_framesPerBuffer = 0;
            m_numChannels = 0;
            m_sampleRate = 0.0f;
        }

        int16_t*    m_buffer;
        size_t      m_framesPerBuffer;
        size_t      m_numChannels;
        float       m_sampleRate;
    };

    static EDemo    s_currentDemo;
    static bool     s_exit;
    static int      s_volumeMultiplier;
    static float    s_lastVolumeMultiplier;
    static bool     s_clippingOn;
    static FILE*    s_recordingWavFile;

    // for recording audio
    static std::mutex                                       s_recordingBuffersMutex;
    static std::queue<std::unique_ptr<SRecordingBuffer>>    s_recordingBuffers;
    static SWaveFileHeader                                  s_waveFileHeader;
    static size_t                                           s_recordedNumSamples;
    static size_t                                           s_recordingNumChannels;
    static size_t                                           s_recordingSampleRate;
    static size_t                                           s_sampleClock;
    static size_t                                           s_numChannels;
    static float                                            s_sampleRate;
};