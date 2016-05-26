//--------------------------------------------------------------------------------------------------
// Main.cpp
//
// This contains all the portaudio setup stuff.  Nothing of major importance really.
// Music.cpp is where all the interesting things are.
//
//--------------------------------------------------------------------------------------------------

#include <stdio.h>
#include "PortAudio/include/portaudio.h"
#include "Music.h"
#include <Windows.h>

// audio data needed by the audio sample generator
float               g_sampleRate = 0.0f;
static const size_t g_numChannels = 2;

//--------------------------------------------------------------------------------------------------
static int GenerateAudioSamples (
    const void *inputBuffer,
    void *outputBuffer,
    unsigned long framesPerBuffer,
    const PaStreamCallbackTimeInfo* timeInfo,
    PaStreamCallbackFlags statusFlags,
    void *userData
) {
    GenerateAudioSamplesCallback((float*)outputBuffer, framesPerBuffer, g_numChannels, g_sampleRate);
    return paContinue;
}

//--------------------------------------------------------------------------------------------------
int main (int argc, char **argv)
{
    // initialize port audio
    PaError err = Pa_Initialize();
    if (err != paNoError)
    {
        printf("Pa_Initialize returned error: %i\n", err);
        return err;
    }

    // figure out what api index WASAPI is
    PaHostApiIndex hostApiIndex = Pa_HostApiTypeIdToHostApiIndex(paWASAPI);
    if (hostApiIndex < 0)
    {
        printf("Pa_HostApiTypeIdToHostApiIndex returned error: %i\n", err);
        return err;
    }

    // get information about the WASAPI host api
    const PaHostApiInfo *hostApiInfo = Pa_GetHostApiInfo(hostApiIndex);
    if (hostApiInfo == nullptr)
    {
        printf("Pa_GetHostApiInfo returned nullptr\n");
        return -1;
    }

    // make sure there's an output device
    if (hostApiInfo->defaultOutputDevice == paNoDevice) {
        fprintf(stderr, "No default output device\n");
        return -1;
    }

    // get device info if we can
    const PaDeviceInfo* deviceInfo = Pa_GetDeviceInfo(hostApiInfo->defaultOutputDevice);
    if (hostApiInfo == nullptr)
    {
        printf("Pa_GetDeviceInfo returned nullptr\n");
        return -1;
    }

    // open a stereo output stream
    PaStreamParameters outputParameters;
    outputParameters.device = hostApiInfo->defaultOutputDevice;
    outputParameters.channelCount = g_numChannels;
    outputParameters.sampleFormat = paFloat32;
    outputParameters.suggestedLatency = deviceInfo->defaultLowOutputLatency;
    outputParameters.hostApiSpecificStreamInfo = nullptr;
    PaStream* stream = nullptr;
    err = Pa_OpenStream(
        &stream,
        nullptr,
        &outputParameters,
        deviceInfo->defaultSampleRate,
        0,
        0,
        GenerateAudioSamples,
        nullptr
    );
    if (err != paNoError) {
        printf("Pa_OpenStream returned error: %i\n", err);
        return err;
    }

    // get the stream info for the stream we created
    const PaStreamInfo* streamInfo = Pa_GetStreamInfo(stream);
    if (streamInfo == nullptr)
    {
        printf("Pa_GetStreamInfo returned nullptr\n");
        return -1;
    }
    g_sampleRate = (float)streamInfo->sampleRate;

    // start the stream
    err = Pa_StartStream(stream);
    if (err != paNoError) {
        printf("Pa_StartStream returned error: %i\n", err);
        return err;
    }

    BYTE keyState1[256];
    BYTE keyState2[256];

    BYTE* oldKeyState = keyState1;
    BYTE* newKeyState = keyState2;

    GetKeyboardState(oldKeyState);



    // main loop for music demos
    MusicDemosMain();

    // stop the stream
    err = Pa_StopStream(stream);
    if (err != paNoError) {
        printf("Pa_StopStream returned error: %i\n", err);
        return err;
    }

    // close the stream
    err = Pa_CloseStream(stream);
    if (err != paNoError) {
        printf("Pa_CloseStream returned error: %i\n", err);
        return err;
    }
    
    // terminate port audio and return success
    Pa_Terminate();
    return 0;
}