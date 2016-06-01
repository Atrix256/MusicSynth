//--------------------------------------------------------------------------------------------------
// DemoMgr.cpp
//
// Handles switching between demos, passing the demos key events, knowing when to exit the app etc.
//
//--------------------------------------------------------------------------------------------------

#include "DemoMgr.h"

EDemo CDemoMgr::s_currentDemo = e_demoFirst;
bool CDemoMgr::s_exit = false;
float CDemoMgr::s_volumeMultiplier = 0.9f;
bool CDemoMgr::s_clippingOn = false;
FILE* CDemoMgr::s_recordingWavFile = nullptr;

// for recording audio
std::mutex CDemoMgr::s_recordingBuffersMutex;
std::queue<std::unique_ptr<CDemoMgr::SRecordingBuffer>> CDemoMgr::s_recordingBuffers;
SWaveFileHeader CDemoMgr::s_waveFileHeader;
size_t CDemoMgr::s_recordedNumSamples;
size_t CDemoMgr::s_recordingNumChannels;
size_t CDemoMgr::s_recordingSampleRate;
size_t CDemoMgr::s_sampleClock; // yes in 32 bit mode this is a uint32 and could roll over, but it would take 27 hours.

//--------------------------------------------------------------------------------------------------
static bool FileExists (const char* fileName) {
    FILE *file = nullptr;
    fopen_s(&file, fileName, "rb");
    if (file) {
        fclose(file);
        return true;
    }
    return false;
}

//--------------------------------------------------------------------------------------------------
void CDemoMgr::StartRecording() {

    // find a filename
    char fileName[256];
    sprintf_s(fileName, 256, "recording.wav");
    int i = 1;
    while (FileExists(fileName)) {
        sprintf_s(fileName, 256, "recording%i.wav", i);
        ++i;
    }

    // open the file for writing
    fopen_s(&s_recordingWavFile, fileName, "w+b");
    if (!s_recordingWavFile) {
        printf("ERROR: could not start recording to %s\r\n", fileName);
        return;
    }

    // write a dummy header for now.  It'll be re-written when we have all our data at the end of
    // the recording process.
    fwrite(&s_waveFileHeader, sizeof(s_waveFileHeader), 1, s_recordingWavFile);

    // remember that we've recorded 0 samples so far
    s_recordedNumSamples = 0;

    // tell the user we've started recording
    printf("Started recording audio to %s\r\n", fileName);
}

//--------------------------------------------------------------------------------------------------
void CDemoMgr::StopRecording() {
    FlushRecordingBuffers();

    // seek to the beginning of the file
    fseek(s_recordingWavFile, 0, SEEK_SET);

    // Fill out the header with the correct information
    s_waveFileHeader.Fill(s_recordedNumSamples, s_recordingNumChannels, s_recordingSampleRate);

    // write the header again, now with correct info
    fwrite(&s_waveFileHeader, sizeof(s_waveFileHeader), 1, s_recordingWavFile);

    // close the file
    fclose(s_recordingWavFile);
    s_recordingWavFile = nullptr;

    // clear any data that may be left in the recording buffers
    ClearRecordingBuffers();

    // tell the user we've stopped recording
    printf("Recording stopped.\r\n");
}

//--------------------------------------------------------------------------------------------------
void CDemoMgr::Update() {
    if (IsRecording())
        FlushRecordingBuffers();
}

//--------------------------------------------------------------------------------------------------
void CDemoMgr::FlushRecordingBuffers() {
    std::lock_guard<std::mutex> guard(s_recordingBuffersMutex);
    while (!s_recordingBuffers.empty()) {
        SRecordingBuffer& buffer = *s_recordingBuffers.front();

        // cache off recording params
        s_recordingNumChannels = buffer.m_numChannels;
        s_recordingSampleRate = size_t(buffer.m_sampleRate);

        // write the samples to disk
        fwrite(buffer.m_buffer, sizeof(int16_t), buffer.m_framesPerBuffer * buffer.m_numChannels, s_recordingWavFile);

        // keep track of how many samples we've recorded
        s_recordedNumSamples += buffer.m_framesPerBuffer * buffer.m_numChannels;

        // pop the buffer now that we've consumed it
        s_recordingBuffers.pop();
    }
}

//--------------------------------------------------------------------------------------------------
void CDemoMgr::ClearRecordingBuffers() {
    std::lock_guard<std::mutex> guard(s_recordingBuffersMutex);
    while (!s_recordingBuffers.empty()) {
        s_recordingBuffers.pop();
    }
}

//--------------------------------------------------------------------------------------------------
void CDemoMgr::AddRecordingBuffer (float *buffer, size_t framesPerBuffer, size_t numChannels, float sampleRate) {

    // make a new SRecordingBuffer and store off the params
    std::unique_ptr<SRecordingBuffer> newBuffer = std::make_unique<SRecordingBuffer>();
    newBuffer->m_framesPerBuffer = framesPerBuffer;
    newBuffer->m_numChannels = numChannels;
    newBuffer->m_sampleRate = sampleRate;

    // convert samples from floats to int16
    newBuffer->m_buffer = new int16_t[framesPerBuffer*numChannels];
    for (size_t i = 0, c = framesPerBuffer*numChannels; i < c; ++i)
        newBuffer->m_buffer[i] = ConvertFloatToAudioSample(buffer[i]);

    // get a lock on the mutex and add this buffer
    std::lock_guard<std::mutex> guard(s_recordingBuffersMutex);
    s_recordingBuffers.push(std::move(newBuffer));
}