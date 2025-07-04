// description: Defines the audio analysis system using PortAudio and Aubio.
// It includes a lock-free ring buffer for real-time audio data transfer
// between a capture thread and an analysis thread.
// developer: ingekastel
// license: GNU General Public License v3.0
// version: 1.2.4
// date: 2025-06-26
// project: Tactical Aim Assist

#pragma once

#include <vector>
#include <atomic>
#include <thread>
#include <string>
#include <memory>

// FIX: Include the actual library headers instead of forward declaring.
// This ensures we have the correct and complete type definitions, resolving
// the 'conflicting declaration' errors.
#include <../portaudio/include/portaudio.h> // Include PortAudio header
#include <../aubio/src/aubio.h> // Include Aubio header

// (The LockFreeRingBuffer class remains unchanged)
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
    AudioManager(uint32_t sample_rate, uint32_t buffer_size, uint32_t hop_size);
    ~AudioManager();

    bool start();
    void stop();
    std::string getLatestAlert();

private:
    // FIX: Corrected the callback signature to exactly match PaStreamCallback.
    // This resolves the 'invalid conversion' error when passing it to Pa_OpenStream.
    static int paCallback(const void* inputBuffer, void* outputBuffer,
                          unsigned long framesPerBuffer,
                          const PaStreamCallbackTimeInfo* timeInfo,
                          PaStreamCallbackFlags statusFlags,
                          void* userData);

    void analysisLoop();

    PaStream* m_stream = nullptr;
    aubio_onset_t* m_onset_obj = nullptr;
    
    const uint32_t m_sample_rate;
    const uint32_t m_buffer_size;
    const uint32_t m_hop_size;
    
    LockFreeRingBuffer<float> m_ring_buffer;
    
    std::thread m_analysis_thread;
    std::atomic<bool> m_running{false};
    
    std::atomic<std::string*> m_last_alert{nullptr};
};