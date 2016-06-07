//--------------------------------------------------------------------------------------------------
// AudioEffects.h
//
// small objects to do basic audio effects
//
//--------------------------------------------------------------------------------------------------
#pragma once

#include <memory>
#include "AudioUtils.h"

// effects available
struct SDelayEffect;
struct SMultiTapReverbEffect;
struct SFlangeEffect;

//--------------------------------------------------------------------------------------------------
struct SDelayEffect {

    SDelayEffect ()
        : m_delayBuffer(nullptr)
        , m_delayBufferSize(0)
        , m_feedback(1.0f)
        , m_sampleIndex(0) {}

    void SetEffectParams (float delayTime, float sampleRate, size_t numChannels, float feedback) {

        size_t numSamples = size_t(delayTime * sampleRate) * numChannels;

        delete[] m_delayBuffer;

        if (numSamples == 0) {
            m_delayBuffer = nullptr;
            return;
        }

        m_delayBuffer = new float[numSamples];
        memset(m_delayBuffer, 0, sizeof(float)*numSamples);

        m_delayBufferSize = numSamples;
        m_feedback = feedback;
        m_sampleIndex = 0;
    }

    float AddSample (float sample) {
        if (!m_delayBuffer)
            return sample;

        // apply feedback in the delay buffer, for whatever is currently in there.
        // also mix in our new sample.
        m_delayBuffer[m_sampleIndex] = m_delayBuffer[m_sampleIndex] * m_feedback + sample;

        // cache off our value to return
        float ret = m_delayBuffer[m_sampleIndex];

        // move the index to the next location
        m_sampleIndex = (m_sampleIndex + 1) % m_delayBufferSize;

        // return the value with echo
        return ret;
    }

    ~SDelayEffect() {
        delete[] m_delayBuffer;
    }

    float*  m_delayBuffer;
    size_t  m_delayBufferSize;
    float   m_feedback;
    size_t  m_sampleIndex;
};

//--------------------------------------------------------------------------------------------------
struct SMultiTapReverbEffect {

    struct STap {
        size_t  m_sampleOffset;
        float   m_volume;
    };

    SMultiTapReverbEffect()
        : m_buffer(nullptr)
        , m_bufferSize(0)
        , m_sampleIndex(0) {}

    void SetEffectParams (float sampleRate, size_t numChannels) {

        m_numChannels = numChannels;
        m_bufferSize = size_t(0.662f * sampleRate) * m_numChannels;
            
        delete[] m_buffer;
        m_buffer = new float[m_bufferSize];

        m_taps[0] = { size_t(0.079f * sampleRate * m_numChannels), 0.0562f };
        m_taps[1] = { size_t(0.130f * sampleRate * m_numChannels), 0.0707f };
        m_taps[2] = { size_t(0.230f * sampleRate * m_numChannels), 0.1778f };
        m_taps[3] = { size_t(0.340f * sampleRate * m_numChannels), 0.0707f };
        m_taps[4] = { size_t(0.470f * sampleRate * m_numChannels), 0.1412f };
        m_taps[5] = { size_t(0.532f * sampleRate * m_numChannels), 0.0891f };
        m_taps[6] = { size_t(0.662f * sampleRate * m_numChannels), 0.2238f };

        ClearBuffer();
    }

    void ClearBuffer (void) {
        memset(m_buffer, 0, sizeof(float)*m_bufferSize);
        m_sampleIndex = 0;
    }

    float AddSample (float sample) {
        // gather all the taps
        float ret = 0.0f;
        for (int i = 0; i < 7; ++i) {
            size_t sampleLoc = (m_sampleIndex + m_taps[i].m_sampleOffset) % m_bufferSize;
            ret += m_buffer[sampleLoc] * m_taps[i].m_volume;
        }

        // move the index to the next location
        m_sampleIndex = (m_sampleIndex + 1) % m_bufferSize;

        // put the sample into the buffer, with feedback
        m_buffer[m_sampleIndex] = sample + ret * 0.5f;

        // return the sum of the taps
        return ret + sample;
    }

    ~SMultiTapReverbEffect() {
        delete[] m_buffer;
    }

    float*      m_buffer;
    size_t      m_numChannels;
    size_t      m_bufferSize;
    size_t      m_sampleIndex;
    STap        m_taps[7];
};

//--------------------------------------------------------------------------------------------------
struct SFlangeEffect {

    struct STap {
        size_t  m_sampleOffset;
        float   m_volume;
    };

    SFlangeEffect()
        : m_buffer(nullptr)
        , m_bufferSize(0)
        , m_sampleIndex(0)
        , m_phase(0.0f)
        , m_phaseAdvance(0.0f) {}

    void SetEffectParams (float sampleRate, size_t numChannels, float frequency, float amplitudeSeconds) {

        m_phase = 0.0f;
        m_phaseAdvance = frequency / sampleRate;

        m_numChannels = numChannels;
        m_bufferSize = size_t(amplitudeSeconds * sampleRate) * m_numChannels;
            
        delete[] m_buffer;
        m_buffer = new float[m_bufferSize];

        ClearBuffer();
    }

    void ClearBuffer (void) {
        memset(m_buffer, 0, sizeof(float)*m_bufferSize);
        m_sampleIndex = 0;
        m_phase = 0.0f;
    }

    float AddSample (float sample) {

        // get the tap, linearly interpolating between samples as appropriate.
        // Our channel data is interleaved, so samples for the same channel are spaced apart by m_numChannels.
        float tapOffsetFloat = (SineWave(m_phase) * 0.5f + 0.5f) * float((m_bufferSize / m_numChannels) - 1);
        float percent = std::fmodf(tapOffsetFloat, 1.0f);
        size_t tapOffset = size_t(tapOffsetFloat) * m_numChannels;
        float tap0 = m_buffer[(m_sampleIndex + tapOffset + m_bufferSize - m_numChannels) % m_bufferSize];
        float tap1 = m_buffer[(m_sampleIndex + tapOffset) % m_bufferSize];
        float tap2 = m_buffer[(m_sampleIndex + tapOffset + m_numChannels) % m_bufferSize];
        float tap3 = m_buffer[(m_sampleIndex + tapOffset + m_numChannels + m_numChannels) % m_bufferSize];
        //float tap = Lerp(tap1, tap2, percent);
        float tap = CubicHermite(tap0, tap1, tap2, tap3, percent);

        // put the sample into the buffer
        m_buffer[m_sampleIndex] = sample;

        // move the index to the next location
        m_sampleIndex = (m_sampleIndex + 1) % m_bufferSize;

        // return the sample mixed with the info from the buffer
        return sample + tap;
    }

    void AdvancePhase () {
        // advance the phase
        m_phase = std::fmodf(m_phase + m_phaseAdvance, 1.0f);
    }

    ~SFlangeEffect() {
        delete[] m_buffer;
    }

    float*      m_buffer;
    size_t      m_numChannels;
    size_t      m_bufferSize;
    size_t      m_sampleIndex;
    float       m_phase;
    float       m_phaseAdvance;
};

//--------------------------------------------------------------------------------------------------
// Generic 2nd order frequency filter
// https://en.wikipedia.org/wiki/Digital_biquad_filter
// http://www.earlevel.com/main/2013/10/13/biquad-calculator-v2/
// http://www.earlevel.com/main/2003/02/28/biquads/
// http://www.earlevel.com/main/2011/01/02/biquad-formulas/
//
struct SBiQuad {

    enum class EType {
        e_lowPass,
        e_highPass,
        e_bandPass,
        e_notch,
        e_peak,
        e_lowShelf,
        e_highShelf
    };

    SBiQuad() {
        m_a0 = m_a1 = m_a2 = 0.0f;
        m_b1 = m_b2 = 0.0f;
        m_xn1 = m_xn2 = 0.0f;
        m_yn1 = m_yn2 = 0.0f;
    }

    // adpated from http://www.earlevel.com/main/2011/01/02/biquad-formulas/
    void SetEffectParams (EType type, float cutoffFrequency, float sampleRate, float Q, float peakGain) {

        // initialize our input and output parameters
        m_xn1 = m_xn2 = 0.0f;
        m_yn1 = m_yn2 = 0.0f;

        // calculate biquad coefficients
        float V = std::powf(10.0f, std::fabs(peakGain) / 20.0f);
        float K = std::tanf(c_pi * cutoffFrequency / sampleRate);
        switch (type) {
            case EType::e_lowPass: {
                float norm = 1 / (1 + K / Q + K * K);
                m_a0 = K * K * norm;
                m_a1 = 2.0f * m_a0;
                m_a2 = m_a0;
                m_b1 = 2.0f * (K * K - 1) * norm;
                m_b2 = (1.0f - K / Q + K * K) * norm;
                break;
            }
            case EType::e_highPass: {
                float norm = 1.0f / (1.0f + K / Q + K * K);
                m_a0 = 1.0f * norm;
                m_a1 = -2.0f * m_a0;
                m_a2 = m_a0;
                m_b1 = 2.0f * (K * K - 1.0f) * norm;
                m_b2 = (1.0f - K / Q + K * K) * norm;
                break;
            }        
            case EType::e_bandPass: {
                float norm = 1.0f / (1.0f + K / Q + K * K);
                m_a0 = K / Q * norm;
                m_a1 = 0.0f;
                m_a2 = -m_a0;
                m_b1 = 2.0f * (K * K - 1.0f) * norm;
                m_b2 = (1.0f - K / Q + K * K) * norm;
                break;
            }
            case EType::e_notch: {
                float norm = 1 / (1 + K / Q + K * K);
                m_a0 = (1 + K * K) * norm;
                m_a1 = 2 * (K * K - 1) * norm;
                m_a2 = m_a0;
                m_b1 = m_a1;
                m_b2 = (1 - K / Q + K * K) * norm;
                break;
            }
            case EType::e_peak: {
                if (peakGain >= 0.0f) {    // boost
                    float norm = 1.0f / (1.0f + 1.0f / Q * K + K * K);
                    m_a0 = (1.0f + V / Q * K + K * K) * norm;
                    m_a1 = 2.0f * (K * K - 1) * norm;
                    m_a2 = (1.0f - V / Q * K + K * K) * norm;
                    m_b1 = m_a1;
                    m_b2 = (1.0f - 1.0f / Q * K + K * K) * norm;
                }
                else {    // cut
                    float norm = 1.0f / (1.0f + V / Q * K + K * K);
                    m_a0 = (1.0f + 1.0f / Q * K + K * K) * norm;
                    m_a1 = 2.0f * (K * K - 1) * norm;
                    m_a2 = (1.0f - 1.0f / Q * K + K * K) * norm;
                    m_b1 = m_a1;
                    m_b2 = (1.0f - V / Q * K + K * K) * norm;
                }
                break;
            }
            case EType::e_lowShelf: {
                if (peakGain >= 0.0f) {    // boost
                    float norm = 1.0f / (1.0f + std::sqrtf(2.0f) * K + K * K);
                    m_a0 = (1.0f + std::sqrtf(2.0f * V) * K + V * K * K) * norm;
                    m_a1 = 2.0f * (V * K * K - 1.0f) * norm;
                    m_a2 = (1.0f - std::sqrtf(2.0f * V) * K + V * K * K) * norm;
                    m_b1 = 2.0f * (K * K - 1.0f) * norm;
                    m_b2 = (1.0f - std::sqrtf(2.0f) * K + K * K) * norm;
                }
                else {    // cut
                    float norm = 1.0f / (1.0f + std::sqrtf(2 * V) * K + V * K * K);
                    m_a0 = (1.0f + std::sqrtf(2.0f) * K + K * K) * norm;
                    m_a1 = 2.0f * (K * K - 1.0f) * norm;
                    m_a2 = (1.0f - std::sqrtf(2.0f) * K + K * K) * norm;
                    m_b1 = 2.0f * (V * K * K - 1.0f) * norm;
                    m_b2 = (1.0f - std::sqrtf(2 * V) * K + V * K * K) * norm;
                }
                break;
            }
            case EType::e_highShelf: {
                if (peakGain >= 0.0f) {    // boost
                    float norm = 1.0f / (1.0f + std::sqrtf(2.0f) * K + K * K);
                    m_a0 = (V + std::sqrtf(2 * V) * K + K * K) * norm;
                    m_a1 = 2.0f * (K * K - V) * norm;
                    m_a2 = (V - std::sqrtf(2 * V) * K + K * K) * norm;
                    m_b1 = 2.0f * (K * K - 1.0f) * norm;
                    m_b2 = (1.0f - std::sqrtf(2.0f) * K + K * K) * norm;
                }
                else {    // cut
                    float norm = 1.0f / (V + std::sqrtf(2.0f * V) * K + K * K);
                    m_a0 = (1.0f + std::sqrtf(2.0f) * K + K * K) * norm;
                    m_a1 = 2.0f * (K * K - 1) * norm;
                    m_a2 = (1.0f - std::sqrtf(2.0f) * K + K * K) * norm;
                    m_b1 = 2.0f * (K * K - V) * norm;
                    m_b2 = (V - std::sqrtf(2.0f * V) * K + K * K) * norm;
                }
                break;
            }
        }
    }

    float AddSample (float x) {

        // calculate the output value
        // y[n] = a0*x[n] + a1*x[n - 1] + a2*x[n - 2] – b1*y[n - 1] – b2*y[n - 2]
        float y = m_a0*x + m_a1 * m_xn1 + m_a2*m_xn2 - m_b1*m_yn1 - m_b2*m_yn2;

        // shift down previous input and output samples
        m_yn2 = m_yn1;
        m_yn1 = y;
        m_xn2 = m_xn1;
        m_xn1 = x;

        // return the output value
        return y;
    }

private:
    // biquad coefficients
    float m_a0;
    float m_a1;
    float m_a2;
    float m_b1;
    float m_b2;

    // previous input samples
    float m_xn1;
    float m_xn2;

    // previous output samples
    float m_yn1;
    float m_yn2;
};
