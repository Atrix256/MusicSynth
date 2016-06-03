//--------------------------------------------------------------------------------------------------
// WavFile.h
//
// Helper stuff for interacting with wave files
//
//--------------------------------------------------------------------------------------------------
#pragma once

#include <inttypes.h>
#include <memory.h>

struct SWaveFileHeader {

    //the main chunk
    unsigned char   m_szChunkID[4];
    uint32_t        m_nChunkSize;
    unsigned char   m_szFormat[4];

    //sub chunk 1 "fmt "
    unsigned char   m_szSubChunk1ID[4];
    uint32_t        m_nSubChunk1Size;
    uint16_t        m_nAudioFormat;
    uint16_t        m_nNumChannels;
    uint32_t        m_nSampleRate;
    uint32_t        m_nByteRate;
    uint16_t        m_nBlockAlign;
    uint16_t        m_nBitsPerSample;

    //sub chunk 2 "data"
    unsigned char   m_szSubChunk2ID[4];
    uint32_t        m_nSubChunk2Size;

    void Fill(int numSamples, int numChannels, int sampleRate)
    {
        //calculate bits per sample and the data size
        int nBitsPerSample = 16;
        int nDataSize = numSamples * 2;

        //fill out the main chunk
        memcpy(m_szChunkID, "RIFF", 4);
        m_nChunkSize = nDataSize + 36;
        memcpy(m_szFormat, "WAVE", 4);

        //fill out sub chunk 1 "fmt "
        memcpy(m_szSubChunk1ID, "fmt ", 4);
        m_nSubChunk1Size = 16;
        m_nAudioFormat = 1;
        m_nNumChannels = numChannels;
        m_nSampleRate = sampleRate;
        m_nByteRate = sampleRate * numChannels * nBitsPerSample / 8;
        m_nBlockAlign = numChannels * nBitsPerSample / 8;
        m_nBitsPerSample = nBitsPerSample;

        //fill out sub chunk 2 "data"
        memcpy(m_szSubChunk2ID, "data", 4);
        m_nSubChunk2Size = nDataSize;
    }
};

#define CLAMP(value,min,max) {if(value < min) { value = min; } else if(value > max) { value = max; }}

//–32,768 to 32,767
inline int16_t ConvertFloatToAudioSample (float in)
{
    in *= 32767.0f;
    CLAMP(in, -32768.0f, 32767.0f);
    return (int16_t)in;
}

// loads in a wave file if it can, allocates memory for the samples and returns that in data.
// if normalizeData is true, it makes all float data be [-1,1] by dividing all values by the absolute value of the largest absolute value of the data
// converts the data to the number of channels and sample rate specified
bool ReadWaveFile (const char *fileName, float *&data, size_t &numSamples, size_t numChannels, size_t sampleRate, bool normalizeData = true);

//--------------------------------------------------------------------------------------------------
struct SWavFile {
public:

    SWavFile () : m_samples(nullptr), m_numSamples(0) { }
    ~SWavFile () {
        if (m_samples) {
            delete[] m_samples;
            m_samples = nullptr;
            m_numSamples = 0;
        }
    }

    bool IsLoaded () const { return m_samples != nullptr; }

    bool Load (const char *fileName, size_t numChannels, size_t sampleRate, bool normalizeData = true) {
        if (m_samples) {
            delete[] m_samples;
            m_samples = nullptr;
            m_numSamples = 0;
        }

        if (!ReadWaveFile(fileName, m_samples, m_numSamples, numChannels, sampleRate, normalizeData))
            return false;

        m_sampleRate = sampleRate;
        m_numChannels = numChannels;
        m_lengthSeconds = float(m_numSamples / m_numChannels) / float(m_sampleRate);
        return true;
    }

    float*  m_samples;
    size_t  m_numSamples;
    size_t  m_sampleRate;
    size_t  m_numChannels;
    float   m_lengthSeconds;
};