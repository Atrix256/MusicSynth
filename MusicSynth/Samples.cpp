//--------------------------------------------------------------------------------------------------
// Samples.cpp
//
// The loaded audio samples
//
//--------------------------------------------------------------------------------------------------

#include "Samples.h"
#include <stdio.h>
#include "DemoMgr.h"

#define SAMPLE(name) SWavFile g_sample_##name;
#include "SampleList.h"

void LoadSamples() {

#define SAMPLE(name) \
    if (!g_sample_##name.Load("Samples/" #name ".wav", CDemoMgr::GetNumChannels(), (size_t)CDemoMgr::GetSampleRate())) \
        printf("Could not load Samples/" #name ".wav.\r\n");
    #include "SampleList.h"
}