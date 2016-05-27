//--------------------------------------------------------------------------------------------------
// DemoMgr.cpp
//
// Handles switching between demos, passing the demos key events, knowing when to exit the app etc.
//
//--------------------------------------------------------------------------------------------------

#include "DemoMgr.h"

EDemo CDemoMgr::s_currentDemo = e_demoFirst;
bool CDemoMgr::s_exit = false;
float CDemoMgr::s_volumeMultiplier = 1.0f;
bool CDemoMgr::s_clippingOn = true;
FILE* CDemoMgr::s_recordingWavFile = nullptr;

// for recording audio
std::mutex CDemoMgr::s_recordingBuffersMutex;
std::queue<CDemoMgr::SRecordingBuffer> CDemoMgr::s_recordingBuffers;

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

    printf("Started recording audio to %s\r\n", fileName);

    // TODO: write dummy header. 
}

//--------------------------------------------------------------------------------------------------
void CDemoMgr::StopRecording() {
    FlushRecordingBuffers();

    // TODO: seek to beginning, write correct header.

    fclose(s_recordingWavFile);
    s_recordingWavFile = nullptr;
    ClearRecordingBuffers();
    printf("Recording stopped.\r\n");
}

//--------------------------------------------------------------------------------------------------
void CDemoMgr::Update() {
    if (IsRecording())
        FlushRecordingBuffers();
}

//--------------------------------------------------------------------------------------------------
void CDemoMgr::FlushRecordingBuffers() {
    // TODO: grab a mutex, write all pending buffers to disk, remove them from the vector.
}

//--------------------------------------------------------------------------------------------------
void CDemoMgr::ClearRecordingBuffers() {
    std::lock_guard<std::mutex> guard(s_recordingBuffersMutex);
    while (!s_recordingBuffers.empty())
        s_recordingBuffers.pop();
}

//--------------------------------------------------------------------------------------------------
void CDemoMgr::AddRecordingBuffer (float *buffer, size_t framesPerBuffer, size_t numChannels, float sampleRate) {

    // copy the buffer data and params
    SRecordingBuffer newBuffer;
    newBuffer.m_buffer = new float[framesPerBuffer*numChannels];
    memcpy(newBuffer.m_buffer, buffer, sizeof(float) * framesPerBuffer * numChannels);
    newBuffer.m_framesPerBuffer = framesPerBuffer;
    newBuffer.m_numChannels = numChannels;
    newBuffer.m_sampleRate = sampleRate;

    // get a lock on the mutex and add this buffer
    std::lock_guard<std::mutex> guard(s_recordingBuffersMutex);
    s_recordingBuffers.push(newBuffer);
}