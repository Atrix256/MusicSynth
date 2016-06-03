//--------------------------------------------------------------------------------------------------
// WavFile.cpp
//
// Helper stuff for interacting with wave files
//
//--------------------------------------------------------------------------------------------------

#include "WavFile.h"
#include "AudioUtils.h"
#include <stdio.h>
#include <memory>

void NormalizeAudioData(float *pData, int nNumSamples)
{
    //figure out what the maximum and minimum value is
    float fMaxValue = pData[0];
    float fMinValue = pData[0];
    for (int nIndex = 0; nIndex < nNumSamples; ++nIndex)
    {
        if (pData[nIndex] > fMaxValue)
        {
            fMaxValue = pData[nIndex];
        }

        if (pData[nIndex] < fMinValue)
        {
            fMinValue = pData[nIndex];
        }
    }

    //calculate the center and the height
    float fCenter = (fMinValue + fMaxValue) / 2.0f;
    float fHeight = fMaxValue - fMinValue;

    //center and normalize the samples
    for (int nIndex = 0; nIndex < nNumSamples; ++nIndex)
    {
        //center the samples
        pData[nIndex] -= fCenter;

        //normalize the samples
        pData[nIndex] /= fHeight;
    }
}

void ChangeNumChannels(float *&pData, int &nNumSamples, int nSrcChannels, int nDestChannels)
{
    //if the number of channels requested is the number of channels already there, or either number of channels is not mono or stereo, return
    if (nSrcChannels == nDestChannels ||
        nSrcChannels < 1 || nSrcChannels > 2 ||
        nDestChannels < 1 || nDestChannels > 2)
    {
        return;
    }

    //if converting to stereo
    if (nDestChannels == 2)
    {
        float *pNewData = new float[nNumSamples * 2];
        for (int nIndex = 0; nIndex < nNumSamples; ++nIndex)
        {
            pNewData[nIndex * 2] = pData[nIndex];
            pNewData[nIndex * 2 + 1] = pData[nIndex];
        }

        delete pData;
        pData = pNewData;
        nNumSamples *= 2;
    }
    //else converting to mono
    else
    {
        float *pNewData = new float[nNumSamples / 2];
        for (int nIndex = 0; nIndex < nNumSamples / 2; ++nIndex)
        {
            pNewData[nIndex] = pData[nIndex * 2] + pData[nIndex * 2 + 1];
        }

        delete pData;
        pData = pNewData;
        nNumSamples /= 2;
    }
}

float GetInterpolatedAudioSample(float *pData, int nNumSamples, float fIndex)
{
    int nIndex1 = (int)fIndex;
    int nIndex0 = nIndex1 - 1;
    int nIndex2 = nIndex1 + 1;
    int nIndex3 = nIndex1 + 2;

    CLAMP(nIndex0, 0, nNumSamples - 1);
    CLAMP(nIndex1, 0, nNumSamples - 1);
    CLAMP(nIndex2, 0, nNumSamples - 1);
    CLAMP(nIndex3, 0, nNumSamples - 1);

    float percent = std::fmodf(fIndex, 1.0f);

    float fSample0 = pData[nIndex0];
    float fSample1 = pData[nIndex1];
    float fSample2 = pData[nIndex2];
    float fSample3 = pData[nIndex3];

    // use cubic hermite interpolation for C1 continuity.  Better than lerp, but not as good as sync of course!
    return CubicHermite(fSample0, fSample1, fSample2, fSample3, percent);
}

void ResampleData(float *&pData, int &nNumSamples, int nSrcSampleRate, int nDestSampleRate)
{
    //if the requested sample rate is the sample rate it already is, bail out and do nothing
    if (nSrcSampleRate == nDestSampleRate)
        return;

    //calculate the ratio of the old sample rate to the new
    float fResampleRatio = (float)nDestSampleRate / (float)nSrcSampleRate;

    //calculate how many samples the new data will have and allocate the new sample data
    int nNewDataNumSamples = (int)((float)nNumSamples * fResampleRatio);
    float *pNewData = new float[nNewDataNumSamples];

    //get each interpolated output sample.
    for (int nIndex = 0; nIndex < nNewDataNumSamples; ++nIndex)
        pNewData[nIndex] = GetInterpolatedAudioSample(pData, nNumSamples, (float)nIndex / fResampleRatio);

    //free the old data and set the new data
    delete[] pData;
    pData = pNewData;
    nNumSamples = nNewDataNumSamples;
}

float PCMToFloat(unsigned char *pPCMData, int nNumBytes)
{
    int32_t nData = 0;
    unsigned char *pData = (unsigned char *)&nData;

    switch (nNumBytes)
    {
        case 1:
        {
            pData[3] = pPCMData[0];
            return ((float)nData) / ((float)0x000000ff);
        }
        case 2:
        {
            pData[2] = pPCMData[0];
            pData[3] = pPCMData[1];
            return ((float)nData) / ((float)0x00007fff);
        }
        case 3:
        {
            pData[1] = pPCMData[0];
            pData[2] = pPCMData[1];
            pData[3] = pPCMData[2];
            return ((float)nData) / ((float)0x007fffff);
        }
        case 4:
        {
            pData[0] = pPCMData[0];
            pData[1] = pPCMData[1];
            pData[2] = pPCMData[2];
            pData[3] = pPCMData[3];
            return ((float)nData) / ((float)0x7fffffff);
        }
        default:
        {
            return 0.0f;
        }
    }
}

bool ReadWaveFile(const char *fileName, float *&data, size_t &numSamples, size_t numChannels, size_t sampleRate, bool normalizeData) {
    //open the file if we can
    FILE *File = nullptr;
    fopen_s(&File, fileName, "rb");
    if (!File)
    {
        return false;
    }

    //read the main chunk ID and make sure it's "RIFF"
    char buffer[5];
    buffer[4] = 0;
    if (fread(buffer, 4, 1, File) != 1 || strcmp(buffer, "RIFF"))
    {
        fclose(File);
        return false;
    }

    //read the main chunk size
    uint32_t nChunkSize;
    if (fread(&nChunkSize, 4, 1, File) != 1)
    {
        fclose(File);
        return false;
    }

    //read the format and make sure it's "WAVE"
    if (fread(buffer, 4, 1, File) != 1 || strcmp(buffer, "WAVE"))
    {
        fclose(File);
        return false;
    }

    long chunkPosFmt = -1;
    long chunkPosData = -1;

    while (chunkPosFmt == -1 || chunkPosData == -1)
    {
        //read a sub chunk id and a chunk size if we can
        if (fread(buffer, 4, 1, File) != 1 || fread(&nChunkSize, 4, 1, File) != 1)
        {
            fclose(File);
            return false;
        }

        //if we hit a fmt
        if (!strcmp(buffer, "fmt "))
        {
            chunkPosFmt = ftell(File) - 8;
        }
        //else if we hit a data
        else if (!strcmp(buffer, "data"))
        {
            chunkPosData = ftell(File) - 8;
        }

        //skip to the next chunk
        fseek(File, nChunkSize, SEEK_CUR);
    }

    //we'll use this handy struct to load in 
    SWaveFileHeader waveData;

    //load the fmt part if we can
    fseek(File, chunkPosFmt, SEEK_SET);
    if (fread(&waveData.m_szSubChunk1ID, 24, 1, File) != 1)
    {
        fclose(File);
        return false;
    }

    //load the data part if we can
    fseek(File, chunkPosData, SEEK_SET);
    if (fread(&waveData.m_szSubChunk2ID, 8, 1, File) != 1)
    {
        fclose(File);
        return false;
    }

    //verify a couple things about the file data
    if (waveData.m_nAudioFormat != 1 ||       //only pcm data
        waveData.m_nNumChannels < 1 ||        //must have a channel
        waveData.m_nNumChannels > 2 ||        //must not have more than 2
        waveData.m_nBitsPerSample > 32 ||     //32 bits per sample max
        waveData.m_nBitsPerSample % 8 != 0 || //must be a multiple of 8 bites
        waveData.m_nBlockAlign > 8)           //blocks must be 8 bytes or lower
    {
        fclose(File);
        return false;
    }

    //figure out how many samples and blocks there are total in the source data
    int nBytesPerBlock = waveData.m_nBlockAlign;
    int nNumBlocks = waveData.m_nSubChunk2Size / nBytesPerBlock;
    int nNumSourceSamples = nNumBlocks * waveData.m_nNumChannels;

    //allocate space for the source samples
    float *pSourceSamples = new float[nNumSourceSamples];

    //maximum size of a block is 8 bytes.  4 bytes per samples, 2 channels
    unsigned char pBlockData[8];
    memset(pBlockData, 0, 8);

    //read in the source samples at whatever sample rate / number of channels it might be in
    int nBytesPerSample = nBytesPerBlock / waveData.m_nNumChannels;
    for (int nIndex = 0; nIndex < nNumSourceSamples; nIndex += waveData.m_nNumChannels)
    {
        //read in a block
        if (fread(pBlockData, waveData.m_nBlockAlign, 1, File) != 1)
        {
            delete[] pSourceSamples;
            fclose(File);
            return false;
        }

        //get the first sample
        pSourceSamples[nIndex] = PCMToFloat(pBlockData, nBytesPerSample);

        //get the second sample if there is one
        if (waveData.m_nNumChannels == 2)
        {
            pSourceSamples[nIndex + 1] = PCMToFloat(&pBlockData[nBytesPerSample], nBytesPerSample);
        }
    }

    //re-sample the sample rate up or down as needed
    ResampleData(pSourceSamples, nNumSourceSamples, waveData.m_nSampleRate, sampleRate);

    //handle switching from mono to stereo or vice versa
    ChangeNumChannels(pSourceSamples, nNumSourceSamples, waveData.m_nNumChannels, numChannels);

    //normalize the data if we should
    if (normalizeData)
        NormalizeAudioData(pSourceSamples, nNumSourceSamples);

    //return our data
    data = pSourceSamples;
    numSamples = nNumSourceSamples;

    return true;
}