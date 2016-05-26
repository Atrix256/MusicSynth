//--------------------------------------------------------------------------------------------------
// Music.h
//
// The music demo logic lives in this file
//
//--------------------------------------------------------------------------------------------------

#pragma once

// This is called by main.cpp to generate audio samples for port audio.  Not main thread.
void GenerateAudioSamplesCallback (float *outputBuffer, size_t framesPerBuffer, size_t numChannels, float sampleRate);

// This is called by main.cpp to do the logic needed to do the demos
void MusicDemosMain ();