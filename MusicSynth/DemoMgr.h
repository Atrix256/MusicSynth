//--------------------------------------------------------------------------------------------------
// DemoMgr.h
//
// Handles switching between demos, passing the demos key events, knowing when to exit the app etc.
//
//--------------------------------------------------------------------------------------------------

#pragma once

#include <stdio.h>
#include "AudioUtils.h"

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
    static const char* s_demoName = #name; \
};
#include "DemoList.h"

//--------------------------------------------------------------------------------------------------
class CDemoMgr {
public:
    inline static void Init () {
        printf("\r\n\r\n\r\n\r\n============================================\r\n");
        printf("Welcome!\r\nUp and down to adjust volume.\r\nLeft and right to change demo.\r\nEscape to exit.\r\n");
        printf("============================================\r\n\r\n");

        // let the demo know we've entered too
        OnEnterDemo();
    }

    inline static void OnEnterDemo () {
        // pass this call onto the current demo
        switch (s_currentDemo) {
            #define DEMO(name) case e_demo##name: Demo##name::OnEnterDemo(); break;
            #include "DemoList.h"
        }
    }

    inline static void GenerateAudioSamples (float *outputBuffer, size_t framesPerBuffer, size_t numChannels, float sampleRate) {
        // pass this call onto the current demo
        switch (s_currentDemo) {
            #define DEMO(name) case e_demo##name: Demo##name::GenerateAudioSamples(outputBuffer, framesPerBuffer, numChannels, sampleRate); break;
            #include "DemoList.h"
        }

        // apply volume adjustment smoothly over the buffer window via a lerp of amplitude.
        // also apply clipping.
        static float lastVolumeMultiplier = 1.0;
        float volumeMultiplier = s_volumeMultiplier;
        for (size_t sample = 0; sample < framesPerBuffer; ++sample, outputBuffer += numChannels) {
            // lerp the volume change across the buffer
            float percent = float(sample) / float(framesPerBuffer);
            float volume = Lerp(lastVolumeMultiplier, volumeMultiplier, percent);

            // apply volume and clipping
            for (size_t channel = 0; channel < numChannels; ++channel) {
                float value = outputBuffer[channel] * volume;
                if (value > 1.0f)
                    value = 1.0f;
                else if (value < -1.0f)
                    value = -1.0f;
                outputBuffer[channel] = value;
            }
        }

        lastVolumeMultiplier = volumeMultiplier;
    }

    static void OnKey(char key, bool pressed) {
        switch (key) {
            // exit when escape is pressed on any demo
            case 27: Exit(); return;
            // when up arrow pressed, increase volume
            case 38: {
                if (pressed) {
                    s_volumeMultiplier += 0.1f;
                    if (s_volumeMultiplier > 1.0f)
                        s_volumeMultiplier = 1.0f;
                }
                return;
            }
            // when down arrow pressed, decrease volume
            case 40: {
                if (pressed) {
                    s_volumeMultiplier -= 0.1f;
                    if (s_volumeMultiplier < 0.0f)
                        s_volumeMultiplier = 0.0f;
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

    static void Exit () {
        s_exit = true;
    }

    static bool WantsExit () { return s_exit; }

private:
    static EDemo    s_currentDemo;
    static bool     s_exit;
    static float    s_volumeMultiplier;
    static float    s_lastVolumeMultiplier;
};