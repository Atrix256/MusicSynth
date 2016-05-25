#include <stdio.h>
#include "PortAudio/include/portaudio.h"
#include <cmath>

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
    static float phase = 0.5f;

    static const float frequency = 1000.0f;
    static const float phaseAdvance = frequency / g_sampleRate;

    phase += phaseAdvance;
    phase = std::fmod(phase, 1.0f);
    
    float *out = (float*)outputBuffer;
    for (unsigned long sample = 0; sample < framesPerBuffer; ++sample, out = &out[g_numChannels]) {
        
        float value = phase * 2.0 - 1.0;

        for (int channel = 0; channel < g_numChannels; ++channel)
            out[channel] = value;
    }

    return paContinue;
    // TODO: make it so you can record this to disk too as a wave file.  You can then show what's going wrong where. You can also fix this problem!
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
        nullptr, /* no input */
        &outputParameters,
        deviceInfo->defaultSampleRate,
        0,
        paClipOff,      /* we won't output out of range samples so don't bother clipping them */
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

    // generate 5 seconds of audio
    static const int NUM_SECONDS = 5;
    printf("Play for %d seconds.\n", NUM_SECONDS);
    Pa_Sleep(NUM_SECONDS * 1000);

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