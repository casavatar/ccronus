// audio_unified.h
// description: Unified audio manager combining basic alerts and advanced FFT analysis
// developer: ingekastel
// license: GNU General Public License v3.0
// version: 3.0.0 - Unified Module
// date: 2025-07-16
// project: Tactical Aim Assist

#pragma once

#include <string>
#include <queue>
#include <mutex>
#include <atomic>
#include <memory>
#include <vector>
#include <unordered_map>
#include <thread>
#include <functional>
#include <chrono>
#include "common_defines.h"

// =============================================================================
// FORWARD DECLARATIONS FOR OPTIONAL FFT SUPPORT
// =============================================================================
#ifdef ENABLE_FFT_ANALYSIS
#include <../portaudio/include/portaudio.h>
#include <../fftw3/api/fftw3.h>

template<typename T>
class LockFreeRingBuffer {
public:
    explicit LockFreeRingBuffer(size_t size);
    ~LockFreeRingBuffer();
    
    bool push(const T& item);
    bool pop(T& item);
    size_t size() const;
    bool empty() const;
    
private:
    std::vector<T> m_buffer;
    std::atomic<size_t> m_head{0};
    std::atomic<size_t> m_tail{0};
    const size_t m_size;
};
#endif

// =============================================================================
// AUDIO STRUCTURES
// =============================================================================
enum class AudioAlertType {
    Info,
    Warning,
    Critical,
    Success,
    Debug
};

enum class AudioAnalysisMode {
    Disabled,
    Basic,
    Advanced,
    Full_Spectrum
};

struct AudioAlert {
    AudioAlertType type;
    std::string message;
    float priority;
    std::chrono::steady_clock::time_point timestamp;
    bool played = false;
    
    AudioAlert(AudioAlertType t, const std::string& msg, float prio = 1.0f)
        : type(t), message(msg), priority(prio), timestamp(std::chrono::steady_clock::now()) {}
};

struct AudioSettings {
    uint32_t sample_rate = DEFAULT_SAMPLE_RATE;
    uint32_t buffer_size = DEFAULT_BUFFER_SIZE;
    uint32_t channels = 2;
    float master_volume = 1.0f;
    bool enable_alerts = true;
    bool enable_fft_analysis = false;
    AudioAnalysisMode analysis_mode = AudioAnalysisMode::Basic;
    uint32_t max_alert_queue_size = 100;
    std::chrono::milliseconds alert_cooldown{500};
};

struct FFTAnalysisResult {
    std::vector<float> frequency_bins;
    std::vector<float> magnitude_spectrum;
    float dominant_frequency = 0.0f;
    float average_amplitude = 0.0f;
    float peak_amplitude = 0.0f;
    bool voice_activity_detected = false;
    std::chrono::steady_clock::time_point timestamp;
};

// =============================================================================
// UNIFIED AUDIO MANAGER CLASS
// =============================================================================
class AudioManager {
public:
    // Constructor/Destructor
    explicit AudioManager(uint32_t sample_rate = DEFAULT_SAMPLE_RATE, 
                         uint32_t buffer_size = DEFAULT_BUFFER_SIZE);
    ~AudioManager();
    
    // Core functionality
    bool start();
    void stop();
    bool isRunning() const;
    bool isInitialized() const;
    
    // Settings management
    void updateSettings(const AudioSettings& settings);
    const AudioSettings& getSettings() const;
    
    // Basic alert system
    void queueAlert(AudioAlertType type, const std::string& message, float priority = 1.0f);
    void queueAlert(const std::string& alertType, float priority = 1.0f); // Legacy compatibility
    std::string getLatestAlert();
    std::vector<AudioAlert> getAllPendingAlerts();
    void clearAlerts();
    size_t getAlertQueueSize() const;
    
    // Advanced FFT analysis (conditionally compiled)
#ifdef ENABLE_FFT_ANALYSIS
    bool startFFTAnalysis();
    void stopFFTAnalysis();
    bool isFFTActive() const;
    FFTAnalysisResult getLatestFFTResult() const;
    void setFFTCallback(std::function<void(const FFTAnalysisResult&)> callback);
#endif
    
    // Audio monitoring
    float getCurrentVolume() const;
    bool isAudioActive() const;
    std::chrono::milliseconds getLastActivityTime() const;
    
    // Diagnostic and debug
    std::vector<std::string> getDiagnosticInfo() const;
    void enableDebugLogging(bool enable);
    
    // Alert type helpers
    void queueInfoAlert(const std::string& message, float priority = 1.0f);
    void queueWarningAlert(const std::string& message, float priority = 2.0f);
    void queueCriticalAlert(const std::string& message, float priority = 3.0f);
    void queueSuccessAlert(const std::string& message, float priority = 1.5f);
    void queueDebugAlert(const std::string& message, float priority = 0.5f);
    
private:
    // Core state
    std::atomic<bool> m_initialized{false};
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_debug_logging{false};
    
    // Settings and configuration
    AudioSettings m_settings;
    mutable std::mutex m_settings_mutex;
    
    // Basic alert system
    std::queue<AudioAlert> m_alert_queue;
    mutable std::mutex m_alert_mutex;
    std::chrono::steady_clock::time_point m_last_alert_time;
    
    // Audio monitoring
    std::atomic<float> m_current_volume{0.0f};
    std::atomic<bool> m_audio_active{false};
    std::chrono::steady_clock::time_point m_last_activity_time;
    
    // Threading
    std::unique_ptr<std::thread> m_processing_thread;
    std::atomic<bool> m_should_stop_processing{false};
    
#ifdef ENABLE_FFT_ANALYSIS
    // FFT Analysis components
    std::atomic<bool> m_fft_active{false};
    std::unique_ptr<LockFreeRingBuffer<float>> m_ring_buffer;
    fftwf_plan m_fft_plan = nullptr;
    fftwf_complex* m_fft_input = nullptr;
    fftwf_complex* m_fft_output = nullptr;
    std::unique_ptr<std::thread> m_fft_thread;
    std::function<void(const FFTAnalysisResult&)> m_fft_callback;
    mutable std::mutex m_fft_mutex;
    FFTAnalysisResult m_latest_fft_result;
    
    // PortAudio components
    PaStream* m_pa_stream = nullptr;
    
    // FFT processing methods
    bool initializeFFT();
    void cleanupFFT();
    void fftProcessingLoop();
    void processAudioBuffer(const float* input, size_t frameCount);
    FFTAnalysisResult performFFTAnalysis(const std::vector<float>& samples);
    static int portAudioCallback(const void* inputBuffer, void* outputBuffer,
                                unsigned long framesPerBuffer,
                                const PaStreamCallbackTimeInfo* timeInfo,
                                PaStreamCallbackFlags statusFlags,
                                void* userData);
#endif
    
    // Helper methods
    void processingLoop();
    void processAlertQueue();
    bool shouldProcessAlert(const AudioAlert& alert) const;
    std::string alertTypeToString(AudioAlertType type) const;
    void updateActivityState();
    void logDebug(const std::string& message) const;
};

// =============================================================================
// TEMPLATE IMPLEMENTATION FOR LOCK-FREE RING BUFFER
// =============================================================================
#ifdef ENABLE_FFT_ANALYSIS
template<typename T>
LockFreeRingBuffer<T>::LockFreeRingBuffer(size_t size) : m_size(size + 1) {
    m_buffer.resize(m_size);
}

template<typename T>
LockFreeRingBuffer<T>::~LockFreeRingBuffer() = default;

template<typename T>
bool LockFreeRingBuffer<T>::push(const T& item) {
    const size_t current_tail = m_tail.load();
    const size_t next_tail = (current_tail + 1) % m_size;
    
    if (next_tail != m_head.load()) {
        m_buffer[current_tail] = item;
        m_tail.store(next_tail);
        return true;
    }
    return false; // Buffer full
}

template<typename T>
bool LockFreeRingBuffer<T>::pop(T& item) {
    const size_t current_head = m_head.load();
    
    if (current_head == m_tail.load()) {
        return false; // Buffer empty
    }
    
    item = m_buffer[current_head];
    m_head.store((current_head + 1) % m_size);
    return true;
}

template<typename T>
size_t LockFreeRingBuffer<T>::size() const {
    const size_t head = m_head.load();
    const size_t tail = m_tail.load();
    return (tail >= head) ? (tail - head) : (m_size - head + tail);
}

template<typename T>
bool LockFreeRingBuffer<T>::empty() const {
    return m_head.load() == m_tail.load();
}
#endif

// =============================================================================
// GLOBAL INSTANCE DECLARATION
// =============================================================================
// Note: This will replace the separate g_audioManager in globals.h