// description: Defines the audio analysis system using PortAudio and Aubio.
// It includes a lock-free ring buffer for real-time audio data transfer
// between a capture thread and an analysis thread.
// developer: ingekastel
// license: GNU General Public License v3.0
// version: 1.2.0
// date: 2025-06-26
// project: Tactical Aim Assist

#pragma once

#include <vector>
#include <atomic>
#include <thread>
#include <string>
#include <memory>

// Forward declarations for PortAudio and Aubio types
typedef struct PaStream PaStream;
typedef struct aubio_onset_t aubio_onset_t;

/**
 * @class LockFreeRingBuffer
 * @brief A simple, single-producer, single-consumer lock-free ring buffer.
 *
 * This buffer is designed for real-time audio data transfer. It uses atomic
 * indices to avoid locks, ensuring low-latency communication between the
 * audio capture callback and the analysis thread.
 */
template<typename T>
class LockFreeRingBuffer {
public:
    explicit LockFreeRingBuffer(size_t capacity) : m_buffer(capacity), m_capacity(capacity) {}

    // Writes data to the buffer. To be called from the producer thread.
    bool write(const T* data, size_t count) {
        const auto write_pos = m_write_index.load(std::memory_order_relaxed);
        const auto read_pos = m_read_index.load(std::memory_order_acquire);

        if (count > m_capacity - (write_pos - read_pos)) {
            return false; // Not enough space
        }

        size_t end1 = std::min(write_pos + count, m_capacity);
        size_t count1 = end1 - write_pos;
        memcpy(&m_buffer[write_pos], data, count1 * sizeof(T));

        if (count1 < count) {
            size_t count2 = count - count1;
            memcpy(&m_buffer[0], data + count1, count2 * sizeof(T));
        }

        m_write_index.store((write_pos + count) % m_capacity, std::memory_order_release);
        return true;
    }

    // Reads data from the buffer. To be called from the consumer thread.
    bool read(T* data, size_t count) {
        const auto read_pos = m_read_index.load(std::memory_order_relaxed);
        const auto write_pos = m_write_index.load(std::memory_order_acquire);

        if (count > write_pos - read_pos) {
            return false; // Not enough data
        }
        
        size_t end1 = std::min(read_pos + count, m_capacity);
        size_t count1 = end1 - read_pos;
        memcpy(data, &m_buffer[read_pos], count1 * sizeof(T));

        if (count1 < count) {
            size_t count2 = count - count1;
            memcpy(data + count1, &m_buffer[0], count2 * sizeof(T));
        }

        m_read_index.store((read_pos + count) % m_capacity, std::memory_order_release);
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

/**
 * @class AudioManager
 * @brief Manages audio capture and real-time analysis for event detection.
 *
 * This class sets up a PortAudio stream in loopback mode to capture system audio.
 * The audio data is then processed in a separate thread using the Aubio library
 * to detect sound onsets, which can be interpreted as game events like footsteps or gunshots.
 */
class AudioManager {
public:
    AudioManager(uint32_t sample_rate, uint32_t buffer_size, uint32_t hop_size);
    ~AudioManager();

    // Starts the audio stream and analysis thread.
    bool start();
    // Stops the audio stream and analysis thread.
    void stop();

    // Returns the latest detected audio alert.
    std::string getLatestAlert();

private:
    // The PortAudio callback function (static as required by the API).
    static int paCallback(const void* inputBuffer, void* outputBuffer,
                          unsigned long framesPerBuffer,
                          const void* timeInfo, unsigned long statusFlags,
                          void* userData);
    
    // The main loop for the analysis thread.
    void analysisLoop();

    // --- Member Variables ---
    PaStream* m_stream = nullptr;
    aubio_onset_t* m_onset_obj = nullptr;
    
    // Configuration for audio processing
    const uint32_t m_sample_rate;
    const uint32_t m_buffer_size;
    const uint32_t m_hop_size;
    
    LockFreeRingBuffer<float> m_ring_buffer;
    
    std::thread m_analysis_thread;
    std::atomic<bool> m_running{false};
    
    // Communication with GUI thread
    std::atomic<std::string*> m_last_alert{nullptr};
};