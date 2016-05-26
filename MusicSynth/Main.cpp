//--------------------------------------------------------------------------------------------------
// Main.cpp
//
// This contains all the portaudio setup stuff.  Nothing of major importance or interest.
//
//--------------------------------------------------------------------------------------------------

#include <stdio.h>
#include "PortAudio/include/portaudio.h"
#include "DemoMgr.h"
#include <algorithm>
#include <Windows.h> // for getting key states

// audio data needed by the audio sample generator
float               g_sampleRate = 0.0f;
static const size_t g_numChannels = 2;

//--------------------------------------------------------------------------------------------------
struct SKeyState {
    SHORT m_keys[256];
};

//--------------------------------------------------------------------------------------------------
static int GenerateAudioSamples (
    const void *inputBuffer,
    void *outputBuffer,
    unsigned long framesPerBuffer,
    const PaStreamCallbackTimeInfo* timeInfo,
    PaStreamCallbackFlags statusFlags,
    void *userData
) {
    CDemoMgr::GenerateAudioSamples((float*)outputBuffer, framesPerBuffer, g_numChannels, g_sampleRate);
    return paContinue;
}

//--------------------------------------------------------------------------------------------------
void GatherKeyStates (SKeyState& state) {
    //GetKeyboardState(state.m_keys);
    for (size_t i = 0; i < 256; ++i) {
        state.m_keys[i] = GetAsyncKeyState(i);
    }
}

//--------------------------------------------------------------------------------------------------
void GenerateKeyEvents (SKeyState& oldState, SKeyState& newState) {
    for (size_t i = 0; i < 256; ++i) {
        if ((oldState.m_keys[i] != 0) != (newState.m_keys[i] != 0)) {
            CDemoMgr::OnKey(char(i), newState.m_keys[i] != 0);
        }
    }
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

    // send key events to the demo manager
    CDemoMgr::Init();
    SKeyState keyState1;
    SKeyState keyState2;
    SKeyState* oldKeyState = &keyState1;
    SKeyState* newKeyState = &keyState2;
    GatherKeyStates(*oldKeyState);
    while (!CDemoMgr::WantsExit()) {
        GatherKeyStates(*newKeyState);
        GenerateKeyEvents(*oldKeyState, *newKeyState);
        std::swap(oldKeyState, newKeyState);
        Sleep(0);
    }

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