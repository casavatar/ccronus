// description: Defines the audio analysis system using PortAudio and FFTW.
// It performs real-time FFT on audio data to detect sound onsets (e.g., gunshots, footsteps).
// developer: ingekastel
// license: GNU General Public License v3.0
// version: 2.6.0
// date: 2025-06-26
// project: Tactical Aim Assist

#pragma once

#include <vector>
#include <atomic>
#include <thread>
#include <string>
#include <memory>
#include <complex>

#include <portaudio.h>
#include <fftw3.h>

// FIX: Wrap the C-style array 'fftwf_complex' in a C++ struct.
// std::vector cannot correctly handle elements that are raw C-style arrays.
// This struct makes the type compatible with standard library containers.
struct Complex {
    float real;
    float imag;
};

// LockFreeRingBuffer class remains the same
template<typename T>
class LockFreeRingBuffer {
public:
    explicit LockFreeRingBuffer(size_t capacity) : m_buffer(capacity), m_capacity(capacity) {}

    bool write(const T* data, size_t count) {
        const auto write_pos = m_write_index.load(std::memory_order_relaxed);
        const auto read_pos = m_read_index.load(std::memory_order_acquire);
        if (count > m_capacity - (write_pos - read_pos)) {
            return false;
        }
        size_t end1 = std::min(write_pos + count, m_capacity);
        size_t count1 = end1 - write_pos;
        memcpy(&m_buffer[write_pos % m_capacity], data, count1 * sizeof(T));
        if (count1 < count) {
            size_t count2 = count - count1;
            memcpy(&m_buffer[0], data + count1, count2 * sizeof(T));
        }
        m_write_index.store(write_pos + count, std::memory_order_release);
        return true;
    }

    bool read(T* data, size_t count) {
        const auto read_pos = m_read_index.load(std::memory_order_relaxed);
        const auto write_pos = m_write_index.load(std::memory_order_acquire);
        if (count > write_pos - read_pos) {
            return false;
        }
        size_t end1 = std::min(read_pos + count, m_capacity);
        size_t count1 = end1 - read_pos;
        memcpy(data, &m_buffer[read_pos % m_capacity], count1 * sizeof(T));
        if (count1 < count) {
            size_t count2 = count - count1;
            memcpy(data + count1, &m_buffer[0], count2 * sizeof(T));
        }
        m_read_index.store(read_pos + count, std::memory_order_release);
        return true;
    }

    size_t size() const {
        return m_write_index.load(std::memory_order_acquire) - m_read_index.load(std::memory_order_acquire);
    }

private:
    std::vector<T> m_buffer;
    size_t m_capacity;
    alignas(64) std::atomic<size_t> m_write_index{0};
    alignas(64) std::atomic<size_t> m_read_index{0};
};

class AudioManager {
public:
    AudioManager(uint32_t sample_rate, uint32_t buffer_size);
    ~AudioManager();

    bool start();
    void stop();
    std::string getLatestAlert();

private:
    static int paCallback(const void* inputBuffer, void* outputBuffer,
                          unsigned long framesPerBuffer,
                          const PaStreamCallbackTimeInfo* timeInfo,
                          PaStreamCallbackFlags statusFlags,
                          void* userData);
    
    void analysisLoop();
    void detectOnset(const std::vector<float>& audio_chunk);

    PaStream* m_stream = nullptr;
    
    fftwf_plan m_fft_plan;
    std::vector<float> m_fft_in;
    std::vector<Complex> m_fft_out;

    const uint32_t m_sample_rate;
    const uint32_t m_buffer_size;
    
    LockFreeRingBuffer<float> m_ring_buffer;
    
    std::thread m_analysis_thread;
    std::atomic<bool> m_running{false};
    
    std::atomic<std::string*> m_last_alert{nullptr};

    float m_previous_energy = 0.0f;
    float m_onset_threshold = 1.5f;
};