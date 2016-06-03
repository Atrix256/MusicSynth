//--------------------------------------------------------------------------------------------------
// Samples.h
//
// The loaded audio samples
//
//--------------------------------------------------------------------------------------------------
#pragma once

#include "WavFile.h"

#define SAMPLE(name) extern SWavFile g_sample_##name;
#include "SampleList.h"

void LoadSamples();