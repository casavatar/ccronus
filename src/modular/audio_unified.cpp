// audio_unified.cpp - COMPLETE FIXED VERSION v3.0.7
// --------------------------------------------------------------------------------------
// description: Unified audio processing system - Fixed FilterType scope
// --------------------------------------------------------------------------------------
// developer: ekastel
//
// license: GNU General Public License v3.0
// version: 3.0.7 - Fixed FilterType declaration
// date: 2025-07-16
// project: Tactical Aim Assist
// license: GNU General Public License v3.0
// --------------------------------------------------------------------------------------

#include <iostream>
#include <windows.h>
#include <atomic>
#include <chrono>
#include <thread>
#include <mutex>
#include <vector>
#include <memory>
#include <cmath>
#include <algorithm>
#include <queue>
#include <string>
#include <unordered_map>
#include <cctype>

#include "globals.h"
#include "core/config.h"

// =============================================================================
// MATHEMATICAL CONSTANTS
// =============================================================================
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// =============================================================================
// AUDIO CONFIGURATION STRUCTURES
// =============================================================================
struct AudioSettings {
    bool enable_processing = false;
    bool enable_directional_audio = true;
    bool enable_fft_analysis = false;
    bool enable_noise_suppression = true;
    double volume_threshold = 0.1;
    double frequency_threshold = 1000.0;
    int sample_rate = 44100;
    int buffer_size = 512;
    int channels = 2;
    double gain = 1.0;
    
    // Audio enhancement settings
    bool enable_bass_boost = false;
    bool enable_treble_boost = false;
    bool enable_compressor = false;
    double bass_frequency = 200.0;
    double treble_frequency = 8000.0;
    double compression_ratio = 2.0;
    
    // Spatial audio settings
    bool enable_3d_audio = false;
    double stereo_width = 1.0;
    double reverb_amount = 0.0;
    
    // Advanced processing
    bool enable_equalizer = false;
    bool enable_limiter = false;
    bool enable_crossfeed = false;
    double limiter_threshold = 0.95;
    double crossfeed_amount = 0.3;
    
    bool operator==(const AudioSettings& other) const {
        return enable_processing == other.enable_processing &&
               enable_directional_audio == other.enable_directional_audio &&
               enable_fft_analysis == other.enable_fft_analysis &&
               enable_noise_suppression == other.enable_noise_suppression &&
               std::abs(volume_threshold - other.volume_threshold) < 0.001 &&
               std::abs(frequency_threshold - other.frequency_threshold) < 0.1 &&
               sample_rate == other.sample_rate &&
               buffer_size == other.buffer_size &&
               channels == other.channels;
    }
    
    bool operator!=(const AudioSettings& other) const {
        return !(*this == other);
    }
};

// =============================================================================
// AUDIO DATA STRUCTURES
// =============================================================================
struct AudioFrame {
    std::vector<float> left_channel;
    std::vector<float> right_channel;
    uint64_t timestamp;
    double peak_level;
    double rms_level;
    double frequency_centroid;
    
    AudioFrame(size_t size = 512) 
        : left_channel(size, 0.0f)
        , right_channel(size, 0.0f)
        , timestamp(0)
        , peak_level(0.0)
        , rms_level(0.0)
        , frequency_centroid(0.0) {}
    
    void clear() {
        std::fill(left_channel.begin(), left_channel.end(), 0.0f);
        std::fill(right_channel.begin(), right_channel.end(), 0.0f);
        peak_level = 0.0;
        rms_level = 0.0;
        frequency_centroid = 0.0;
    }
    
    void calculateLevels() {
        peak_level = 0.0;
        double sum_squares = 0.0;
        size_t total_samples = 0;
        
        for (float sample : left_channel) {
            peak_level = std::max(peak_level, static_cast<double>(std::abs(sample)));
            sum_squares += sample * sample;
            total_samples++;
        }
        
        for (float sample : right_channel) {
            peak_level = std::max(peak_level, static_cast<double>(std::abs(sample)));
            sum_squares += sample * sample;
            total_samples++;
        }
        
        rms_level = total_samples > 0 ? std::sqrt(sum_squares / total_samples) : 0.0;
        
        // Simple frequency analysis (centroid approximation)
        calculateFrequencyCentroid();
    }
    
private:
    void calculateFrequencyCentroid() {
        // Simple spectral centroid calculation
        double weighted_sum = 0.0;
        double magnitude_sum = 0.0;
        
        for (size_t i = 1; i < left_channel.size() / 2; ++i) {
            double magnitude = std::abs(left_channel[i]) + std::abs(right_channel[i]);
            double frequency = static_cast<double>(i) * 44100.0 / left_channel.size();
            
            weighted_sum += frequency * magnitude;
            magnitude_sum += magnitude;
        }
        
        frequency_centroid = magnitude_sum > 0 ? weighted_sum / magnitude_sum : 0.0;
    }
};

// =============================================================================
// AUDIO FILTER CLASSES
// =============================================================================
class SimpleFilter {
public:
    enum FilterType { LOWPASS, HIGHPASS, BANDPASS };  // FIXED: Moved enum to public
    
private:
    double m_cutoff;
    double m_resonance;
    double m_sample_rate;
    double m_x1, m_x2, m_y1, m_y2; // Filter memory
    FilterType m_type;  // FIXED: Now can use FilterType since it's declared above
    
public:
    // FIXED: Correct initialization order
    SimpleFilter(FilterType type, double cutoff, double resonance = 1.0, double sample_rate = 44100.0)
        : m_cutoff(cutoff), m_resonance(resonance), m_sample_rate(sample_rate)
        , m_x1(0), m_x2(0), m_y1(0), m_y2(0), m_type(type) {}
    
    float process(float input) {
        // Simple biquad filter implementation
        double frequency = 2.0 * M_PI * m_cutoff / m_sample_rate;
        double q = m_resonance;
        
        double cos_freq = std::cos(frequency);
        double sin_freq = std::sin(frequency);
        double alpha = sin_freq / (2.0 * q);
        
        double b0, b1, b2, a0, a1, a2;
        
        switch (m_type) {
            case LOWPASS:
                b0 = (1.0 - cos_freq) / 2.0;
                b1 = 1.0 - cos_freq;
                b2 = (1.0 - cos_freq) / 2.0;
                a0 = 1.0 + alpha;
                a1 = -2.0 * cos_freq;
                a2 = 1.0 - alpha;
                break;
                
            case HIGHPASS:
                b0 = (1.0 + cos_freq) / 2.0;
                b1 = -(1.0 + cos_freq);
                b2 = (1.0 + cos_freq) / 2.0;
                a0 = 1.0 + alpha;
                a1 = -2.0 * cos_freq;
                a2 = 1.0 - alpha;
                break;
                
            case BANDPASS:
                b0 = alpha;
                b1 = 0.0;
                b2 = -alpha;
                a0 = 1.0 + alpha;
                a1 = -2.0 * cos_freq;
                a2 = 1.0 - alpha;
                break;
        }
        
        // Normalize coefficients
        b0 /= a0;
        b1 /= a0;
        b2 /= a0;
        a1 /= a0;
        a2 /= a0;
        
        // Apply filter
        double output = b0 * input + b1 * m_x1 + b2 * m_x2 - a1 * m_y1 - a2 * m_y2;
        
        // Update delay line
        m_x2 = m_x1;
        m_x1 = input;
        m_y2 = m_y1;
        m_y1 = output;
        
        return static_cast<float>(output);
    }
    
    void reset() {
        m_x1 = m_x2 = m_y1 = m_y2 = 0.0;
    }
    
    void setCutoff(double cutoff) {
        m_cutoff = cutoff;
    }
    
    void setResonance(double resonance) {
        m_resonance = resonance;
    }
    
    FilterType getType() const {
        return m_type;
    }
    
    double getCutoff() const {
        return m_cutoff;
    }
};

class Equalizer {
private:
    std::vector<SimpleFilter> m_bands;
    std::vector<double> m_gains;
    
public:
    Equalizer() {
        // Standard 5-band EQ: 60Hz, 200Hz, 1kHz, 5kHz, 15kHz
        m_bands.emplace_back(SimpleFilter::BANDPASS, 60.0, 0.7);
        m_bands.emplace_back(SimpleFilter::BANDPASS, 200.0, 0.7);
        m_bands.emplace_back(SimpleFilter::BANDPASS, 1000.0, 0.7);
        m_bands.emplace_back(SimpleFilter::BANDPASS, 5000.0, 0.7);
        m_bands.emplace_back(SimpleFilter::BANDPASS, 15000.0, 0.7);
        
        m_gains.resize(5, 1.0); // Unity gain by default
    }
    
    void setGain(size_t band, double gain) {
        if (band < m_gains.size()) {
            m_gains[band] = gain;
        }
    }
    
    double getGain(size_t band) const {
        if (band < m_gains.size()) {
            return m_gains[band];
        }
        return 1.0;
    }
    
    size_t getBandCount() const {
        return m_bands.size();
    }
    
    void setBandFrequency(size_t band, double frequency) {
        if (band < m_bands.size()) {
            m_bands[band].setCutoff(frequency);
        }
    }
    
    float process(float input) {
        float output = 0.0f;
        
        for (size_t i = 0; i < m_bands.size(); ++i) {
            output += m_bands[i].process(input) * static_cast<float>(m_gains[i]);
        }
        
        return output * 0.2f; // Scale down to prevent clipping
    }
    
    void reset() {
        for (auto& band : m_bands) {
            band.reset();
        }
    }
    
    // Preset configurations
    void setPreset(const std::string& presetName) {
        if (presetName == "flat") {
            for (size_t i = 0; i < m_gains.size(); ++i) {
                m_gains[i] = 1.0;
            }
        } else if (presetName == "bass_boost") {
            m_gains[0] = 1.5; // 60Hz
            m_gains[1] = 1.3; // 200Hz
            m_gains[2] = 1.0; // 1kHz
            m_gains[3] = 1.0; // 5kHz
            m_gains[4] = 1.0; // 15kHz
        } else if (presetName == "treble_boost") {
            m_gains[0] = 1.0; // 60Hz
            m_gains[1] = 1.0; // 200Hz
            m_gains[2] = 1.0; // 1kHz
            m_gains[3] = 1.3; // 5kHz
            m_gains[4] = 1.5; // 15kHz
        } else if (presetName == "v_shape") {
            m_gains[0] = 1.4; // 60Hz
            m_gains[1] = 1.2; // 200Hz
            m_gains[2] = 0.8; // 1kHz
            m_gains[3] = 1.2; // 5kHz
            m_gains[4] = 1.4; // 15kHz
        }
    }
};

// =============================================================================
// AUDIO MANAGER CLASS
// =============================================================================
class AudioManager {
private:
    AudioSettings m_settings;
    std::atomic<bool> m_isRunning{false};
    std::atomic<bool> m_isProcessing{false};
    std::thread m_processingThread;
    mutable std::mutex m_settingsMutex;
    mutable std::mutex m_dataMutex;
    
    // Audio buffers
    std::vector<AudioFrame> m_audioBuffer;
    size_t m_bufferWriteIndex = 0;
    size_t m_bufferReadIndex = 0;
    static constexpr size_t BUFFER_COUNT = 4;
    
    // Audio analysis data
    std::atomic<double> m_currentVolume{0.0};
    std::atomic<double> m_peakVolume{0.0};
    std::atomic<double> m_averageVolume{0.0};
    std::atomic<uint64_t> m_samplesProcessed{0};
    std::atomic<double> m_frequencyCentroid{0.0};
    
    // Performance monitoring
    std::atomic<uint64_t> m_lastProcessTime{0};
    std::atomic<double> m_processingLoad{0.0};
    std::atomic<uint64_t> m_underruns{0};
    std::atomic<uint64_t> m_overruns{0};
    
    // Audio processing components
    std::unique_ptr<Equalizer> m_equalizer;
    std::unique_ptr<SimpleFilter> m_bassFilter;
    std::unique_ptr<SimpleFilter> m_trebleFilter;
    
    // Crossfeed processing
    struct CrossfeedState {
        double delay_left = 0.0;
        double delay_right = 0.0;
        double filter_left = 0.0;
        double filter_right = 0.0;
    } m_crossfeed;
    
public:
    AudioManager() {
        // Initialize audio buffer
        m_audioBuffer.resize(BUFFER_COUNT);
        for (auto& frame : m_audioBuffer) {
            frame = AudioFrame(m_settings.buffer_size);
        }
        
        // Initialize audio processors
        m_equalizer = std::make_unique<Equalizer>();
        m_bassFilter = std::make_unique<SimpleFilter>(SimpleFilter::LOWPASS, 200.0, 0.7);
        m_trebleFilter = std::make_unique<SimpleFilter>(SimpleFilter::HIGHPASS, 8000.0, 0.7);
    }
    
    ~AudioManager() {
        stop();
    }
    
    bool start() {
        if (m_isRunning.load()) {
            logMessage("AudioManager already running");
            return true;
        }
        
        logMessage("🎵 Starting AudioManager...");
        
        // Validate settings
        if (!validateSettings()) {
            logMessage("ERROR: Invalid audio settings");
            return false;
        }
        
        // Initialize audio components
        if (!initializeComponents()) {
            logMessage("ERROR: Failed to initialize audio components");
            return false;
        }
        
        // Start processing thread
        m_isRunning.store(true);
        m_processingThread = std::thread(&AudioManager::processingLoop, this);
        
        logMessage("✅ AudioManager started successfully");
        return true;
    }
    
    void stop() {
        if (!m_isRunning.load()) return;
        
        logMessage("🛑 Stopping AudioManager...");
        
        m_isRunning.store(false);
        m_isProcessing.store(false);
        
        if (m_processingThread.joinable()) {
            m_processingThread.join();
        }
        
        shutdownComponents();
        
        logMessage("✅ AudioManager stopped");
    }
    
    void updateSettings(const AudioSettings& settings) {
        std::lock_guard<std::mutex> lock(m_settingsMutex);
        
        bool restart_needed = (m_settings.sample_rate != settings.sample_rate ||
                              m_settings.buffer_size != settings.buffer_size ||
                              m_settings.channels != settings.channels);
        
        bool processing_changed = (m_settings.enable_processing != settings.enable_processing);
        
        bool fft_state_changed = (m_settings.enable_fft_analysis != settings.enable_fft_analysis);
        if (fft_state_changed) {
            logDebug("FFT analysis state changed: " + std::string(settings.enable_fft_analysis ? "enabled" : "disabled"));
        }
        
        // Update filter settings if needed
        if (m_settings.bass_frequency != settings.bass_frequency) {
            m_bassFilter = std::make_unique<SimpleFilter>(SimpleFilter::LOWPASS, settings.bass_frequency, 0.7, settings.sample_rate);
        }
        
        if (m_settings.treble_frequency != settings.treble_frequency) {
            m_trebleFilter = std::make_unique<SimpleFilter>(SimpleFilter::HIGHPASS, settings.treble_frequency, 0.7, settings.sample_rate);
        }
        
        m_settings = settings;
        
        if (restart_needed && m_isRunning.load()) {
            logMessage("Audio settings changed, restarting...");
            stop();
            start();
        } else if (processing_changed) {
            logMessage("Audio processing " + std::string(settings.enable_processing ? "enabled" : "disabled"));
        }
        
        logDebug("Audio settings updated");
    }
    
    AudioSettings getSettings() const {
        std::lock_guard<std::mutex> lock(m_settingsMutex);
        return m_settings;
    }
    
    // Audio level monitoring
    double getCurrentVolume() const { return m_currentVolume.load(); }
    double getPeakVolume() const { return m_peakVolume.load(); }
    double getAverageVolume() const { return m_averageVolume.load(); }
    double getFrequencyCentroid() const { return m_frequencyCentroid.load(); }
    uint64_t getSamplesProcessed() const { return m_samplesProcessed.load(); }
    double getProcessingLoad() const { return m_processingLoad.load(); }
    uint64_t getUnderruns() const { return m_underruns.load(); }
    uint64_t getOverruns() const { return m_overruns.load(); }
    
    bool isRunning() const { return m_isRunning.load(); }
    bool isProcessing() const { return m_isProcessing.load(); }
    
    void resetStats() {
        m_samplesProcessed.store(0);
        m_peakVolume.store(0.0);
        m_averageVolume.store(0.0);
        m_processingLoad.store(0.0);
        m_underruns.store(0);
        m_overruns.store(0);
        m_frequencyCentroid.store(0.0);
    }
    
    // Equalizer control methods
    void setEqualizerPreset(const std::string& presetName) {
        if (m_equalizer) {
            m_equalizer->setPreset(presetName);
            logDebug("Applied equalizer preset: " + presetName);
        }
    }
    
    void setEqualizerBandGain(size_t band, double gain) {
        if (m_equalizer) {
            m_equalizer->setGain(band, gain);
            logDebug("Set EQ band " + std::to_string(band) + " gain to " + std::to_string(gain));
        }
    }
    
    double getEqualizerBandGain(size_t band) const {
        if (m_equalizer) {
            return m_equalizer->getGain(band);
        }
        return 1.0;
    }
    
    size_t getEqualizerBandCount() const {
        if (m_equalizer) {
            return m_equalizer->getBandCount();
        }
        return 0;
    }
    
    // Audio data injection for testing
    void injectAudioData(const std::vector<float>& leftChannel, const std::vector<float>& rightChannel) {
        if (!m_isRunning.load() || leftChannel.size() != rightChannel.size()) return;
        
        std::lock_guard<std::mutex> lock(m_dataMutex);
        
        // Check for buffer overrun
        size_t nextWriteIndex = (m_bufferWriteIndex + 1) % BUFFER_COUNT;
        if (nextWriteIndex == m_bufferReadIndex) {
            m_overruns.fetch_add(1);
            return; // Buffer full
        }
        
        AudioFrame& frame = m_audioBuffer[m_bufferWriteIndex];
        
        size_t samples = std::min(leftChannel.size(), frame.left_channel.size());
        std::copy_n(leftChannel.begin(), samples, frame.left_channel.begin());
        std::copy_n(rightChannel.begin(), samples, frame.right_channel.begin());
        
        frame.timestamp = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
        
        frame.calculateLevels();
        
        m_bufferWriteIndex = nextWriteIndex;
    }
    
    std::vector<double> getFrequencySpectrum() const {
        std::lock_guard<std::mutex> lock(m_dataMutex);
        
        if (m_bufferReadIndex == m_bufferWriteIndex) {
            return std::vector<double>(256, 0.0); // Return empty spectrum
        }
        
        const AudioFrame& frame = m_audioBuffer[m_bufferReadIndex];
        return performFFT(frame);
    }
    
    double getStereoWidth() const {
        std::lock_guard<std::mutex> lock(m_dataMutex);
        
        if (m_bufferReadIndex == m_bufferWriteIndex) return 0.0;
        
        const AudioFrame& frame = m_audioBuffer[m_bufferReadIndex];
        return calculateStereoWidth(frame);
    }
    
    std::string getAudioDirection() const {
        double stereoWidth = getStereoWidth();
        
        if (stereoWidth > 0.1) return "right";
        else if (stereoWidth < -0.1) return "left";
        else return "center";
    }
    
private:
    bool validateSettings() const {
        return m_settings.sample_rate > 0 && 
               m_settings.buffer_size > 0 && 
               m_settings.channels > 0 &&
               m_settings.volume_threshold >= 0.0 &&
               m_settings.gain > 0.0;
    }
    
    bool initializeComponents() {
        logDebug("Initializing audio components...");
        
        // Initialize audio buffers
        for (auto& frame : m_audioBuffer) {
            frame = AudioFrame(m_settings.buffer_size);
        }
        
        // Reset indices
        m_bufferWriteIndex = 0;
        m_bufferReadIndex = 0;
        
        // Reset audio processors
        if (m_equalizer) m_equalizer->reset();
        if (m_bassFilter) m_bassFilter->reset();
        if (m_trebleFilter) m_trebleFilter->reset();
        
        // Reset crossfeed state
        m_crossfeed = CrossfeedState{};
        
        // Reset stats
        resetStats();
        
        logDebug("Audio components initialized");
        return true;
    }
    
    void shutdownComponents() {
        logDebug("Shutting down audio components...");
        
        // Clear buffers
        for (auto& frame : m_audioBuffer) {
            frame.clear();
        }
        
        logDebug("Audio components shutdown complete");
    }
    
    void processAudioFrame(AudioFrame& frame) {
        if (!m_settings.enable_processing) return;
        
        // Apply gain
        if (m_settings.gain != 1.0) {
            applyGain(frame, m_settings.gain);
        }
        
        // Noise suppression
        if (m_settings.enable_noise_suppression) {
            applyNoiseGate(frame);
        }
        
        // Equalizer
        if (m_settings.enable_equalizer && m_equalizer) {
            applyEqualizer(frame);
        }
        
        // Bass boost
        if (m_settings.enable_bass_boost && m_bassFilter) {
            applyBassBoost(frame);
        }
        
        // Treble boost
        if (m_settings.enable_treble_boost && m_trebleFilter) {
            applyTrebleBoost(frame);
        }
        
        // Compression
        if (m_settings.enable_compressor) {
            applyCompression(frame);
        }
        
        // 3D audio processing
        if (m_settings.enable_3d_audio) {
            apply3DAudio(frame);
        }
        
        // Crossfeed
        if (m_settings.enable_crossfeed) {
            applyCrossfeed(frame);
        }
        
        // Limiter (always last)
        if (m_settings.enable_limiter) {
            applyLimiter(frame);
        }
        
        // Recalculate levels after processing
        frame.calculateLevels();
    }
    
    void applyGain(AudioFrame& frame, double gain) {
        float gain_f = static_cast<float>(gain);
        
        for (float& sample : frame.left_channel) {
            sample *= gain_f;
        }
        for (float& sample : frame.right_channel) {
            sample *= gain_f;
        }
    }
    
    void applyNoiseGate(AudioFrame& frame) {
        float threshold = static_cast<float>(m_settings.volume_threshold);
        
        for (float& sample : frame.left_channel) {
            if (std::abs(sample) < threshold) {
                sample *= 0.1f; // Reduce low-level noise
            }
        }
        for (float& sample : frame.right_channel) {
            if (std::abs(sample) < threshold) {
                sample *= 0.1f;
            }
        }
    }
    
    void applyEqualizer(AudioFrame& frame) {
        for (float& sample : frame.left_channel) {
            sample = m_equalizer->process(sample);
        }
        for (float& sample : frame.right_channel) {
            sample = m_equalizer->process(sample);
        }
    }
    
    void applyBassBoost(AudioFrame& frame) {
        for (float& sample : frame.left_channel) {
            float filtered = m_bassFilter->process(sample);
            sample = sample + filtered * 0.3f; // Mix with original
        }
        for (float& sample : frame.right_channel) {
            float filtered = m_bassFilter->process(sample);
            sample = sample + filtered * 0.3f;
        }
    }
    
    void applyTrebleBoost(AudioFrame& frame) {
        for (float& sample : frame.left_channel) {
            float filtered = m_trebleFilter->process(sample);
            sample = sample + filtered * 0.2f; // Mix with original
        }
        for (float& sample : frame.right_channel) {
            float filtered = m_trebleFilter->process(sample);
            sample = sample + filtered * 0.2f;
        }
    }
    
    void applyCompression(AudioFrame& frame) {
        double ratio = m_settings.compression_ratio;
        double threshold = 0.7;
        double attack = 0.003; // 3ms attack
        double release = 0.1;  // 100ms release
        
        static double envelope_left = 0.0;
        static double envelope_right = 0.0;
        
        for (size_t i = 0; i < frame.left_channel.size(); ++i) {
            // Left channel
            double input_level = std::abs(frame.left_channel[i]);
            double target_envelope = input_level > threshold ? input_level : 0.0;
            
            if (target_envelope > envelope_left) {
                envelope_left += (target_envelope - envelope_left) * attack;
            } else {
                envelope_left += (target_envelope - envelope_left) * release;
            }
            
            if (envelope_left > threshold) {
                double excess = envelope_left - threshold;
                double compressed_excess = excess / ratio;
                double gain_reduction = (threshold + compressed_excess) / envelope_left;
                frame.left_channel[i] *= static_cast<float>(gain_reduction);
            }
            
            // Right channel
            input_level = std::abs(frame.right_channel[i]);
            target_envelope = input_level > threshold ? input_level : 0.0;
            
            if (target_envelope > envelope_right) {
                envelope_right += (target_envelope - envelope_right) * attack;
            } else {
                envelope_right += (target_envelope - envelope_right) * release;
            }
            
            if (envelope_right > threshold) {
                double excess = envelope_right - threshold;
                double compressed_excess = excess / ratio;
                double gain_reduction = (threshold + compressed_excess) / envelope_right;
                frame.right_channel[i] *= static_cast<float>(gain_reduction);
            }
        }
    }
    
    void apply3DAudio(AudioFrame& frame) {
        double width = m_settings.stereo_width;
        
        for (size_t i = 0; i < frame.left_channel.size(); ++i) {
            float left = frame.left_channel[i];
            float right = frame.right_channel[i];
            
            float mid = (left + right) * 0.5f;
            float side = (left - right) * 0.5f;
            
            frame.left_channel[i] = mid + side * static_cast<float>(width);
            frame.right_channel[i] = mid - side * static_cast<float>(width);
        }
    }
    
    void applyCrossfeed(AudioFrame& frame) {
        double amount = m_settings.crossfeed_amount;
        
        for (size_t i = 0; i < frame.left_channel.size(); ++i) {
            float left = frame.left_channel[i];
            float right = frame.right_channel[i];
            
            // Apply crossfeed with filtering
            double filtered_left = left * 0.7 + m_crossfeed.filter_left * 0.3;
            double filtered_right = right * 0.7 + m_crossfeed.filter_right * 0.3;
            
            m_crossfeed.filter_left = filtered_left;
            m_crossfeed.filter_right = filtered_right;
            
            frame.left_channel[i] = static_cast<float>(left + filtered_right * amount);
            frame.right_channel[i] = static_cast<float>(right + filtered_left * amount);
        }
    }
    
    void applyLimiter(AudioFrame& frame) {
        double threshold = m_settings.limiter_threshold;
        
        for (float& sample : frame.left_channel) {
            if (sample > threshold) {
                sample = static_cast<float>(threshold);
            } else if (sample < -threshold) {
                sample = static_cast<float>(-threshold);
            }
        }
        
        for (float& sample : frame.right_channel) {
            if (sample > threshold) {
                sample = static_cast<float>(threshold);
            } else if (sample < -threshold) {
                sample = static_cast<float>(-threshold);
            }
        }
    }
    
    void updateStatistics(const AudioFrame& frame) {
        m_currentVolume.store(frame.rms_level);
        m_frequencyCentroid.store(frame.frequency_centroid);
        
        double currentPeak = m_peakVolume.load();
        if (frame.peak_level > currentPeak) {
            m_peakVolume.store(frame.peak_level);
        }
        
        // Update running average
        static double runningSum = 0.0;
        static uint64_t sampleCount = 0;
        
        runningSum += frame.rms_level;
        sampleCount++;
        m_averageVolume.store(runningSum / sampleCount);
        
        m_samplesProcessed.fetch_add(frame.left_channel.size());
    }
    
    void processingLoop() {
        logDebug("Audio processing loop started");
        m_isProcessing.store(true);
        
        while (m_isRunning.load()) {
            auto frameStart = std::chrono::high_resolution_clock::now();
            
            // Process available audio frames
            bool processedFrame = false;
            
            {
                std::lock_guard<std::mutex> lock(m_dataMutex);
                
                if (m_bufferReadIndex != m_bufferWriteIndex) {
                    AudioFrame& frame = m_audioBuffer[m_bufferReadIndex];
                    
                    // Process the frame
                    processAudioFrame(frame);
                    
                    // Update statistics
                    updateStatistics(frame);
                    
                    m_bufferReadIndex = (m_bufferReadIndex + 1) % BUFFER_COUNT;
                    processedFrame = true;
                } else {
                    // Check for underrun
                    m_underruns.fetch_add(1);
                }
            }
            
            // Calculate processing load
            auto frameEnd = std::chrono::high_resolution_clock::now();
            auto processingTime = std::chrono::duration<double, std::micro>(frameEnd - frameStart).count();
            auto targetTime = 1000000.0 / (m_settings.sample_rate / m_settings.buffer_size);
            
            double load = processingTime / targetTime;
            m_processingLoad.store(load);
            
            m_lastProcessTime.store(std::chrono::duration_cast<std::chrono::microseconds>(
                frameEnd.time_since_epoch()).count());
            
            if (!processedFrame) {
                // No frame to process, sleep briefly
                std::this_thread::sleep_for(std::chrono::microseconds(100));
            }
            
            // Yield if processing load is too high
            if (load > 0.8) {
                std::this_thread::sleep_for(std::chrono::microseconds(50));
            }
        }
        
        m_isProcessing.store(false);
        logDebug("Audio processing loop ended");
    }
    
    // Advanced analysis functions
    std::vector<double> performFFT(const AudioFrame& frame) const {
        // Simple magnitude spectrum calculation (not real FFT)
        std::vector<double> spectrum(256, 0.0);
        
        size_t bins = std::min(spectrum.size(), frame.left_channel.size() / 2);
        
        for (size_t i = 0; i < bins; ++i) {
            double left_mag = std::abs(frame.left_channel[i]);
            double right_mag = std::abs(frame.right_channel[i]);
            spectrum[i] = (left_mag + right_mag) * 0.5;
        }
        
        return spectrum;
    }
    
    double calculateStereoWidth(const AudioFrame& frame) const {
        double left_energy = 0.0;
        double right_energy = 0.0;
        
        for (size_t i = 0; i < frame.left_channel.size(); ++i) {
            left_energy += frame.left_channel[i] * frame.left_channel[i];
            right_energy += frame.right_channel[i] * frame.right_channel[i];
        }
        
        double total_energy = left_energy + right_energy;
        if (total_energy > 0.0) {
            return (right_energy - left_energy) / total_energy;
        }
        
        return 0.0;
    }
};

// =============================================================================
// GLOBAL AUDIO MANAGER INSTANCE
// =============================================================================
std::unique_ptr<AudioManager> g_audioManager;

// =============================================================================
// AUDIO SYSTEM INTERFACE FUNCTIONS
// =============================================================================
bool initializeAudioSystem() {
    logMessage("🎵 Initializing Audio System...");
    
    if (g_audioManager) {
        logMessage("Audio system already initialized");
        return true;
    }
    
    g_audioManager = std::make_unique<AudioManager>();
    if (!g_audioManager) {
        logMessage("❌ Failed to create AudioManager");
        return false;
    }
    
    // Set default settings
    AudioSettings defaultSettings;
    defaultSettings.enable_processing = true;
    defaultSettings.enable_directional_audio = true;
    defaultSettings.enable_noise_suppression = true;
    defaultSettings.volume_threshold = 0.05;
    defaultSettings.gain = 1.0;
    
    g_audioManager->updateSettings(defaultSettings);
    
    if (!g_audioManager->start()) {
        logMessage("❌ Failed to start AudioManager");
        g_audioManager.reset();
        return false;
    }
    
    logMessage("✅ Audio System initialized successfully");
    return true;
}

void shutdownAudioSystem() {
    logMessage("🛑 Shutting down Audio System...");
    
    if (g_audioManager) {
        g_audioManager->stop();
        g_audioManager.reset();
    }
    
    logMessage("✅ Audio System shutdown complete");
}

AudioManager* getAudioManager() {
    return g_audioManager.get();
}

// =============================================================================
// AUDIO SYSTEM STATUS AND DIAGNOSTICS
// =============================================================================
void printAudioSystemStatus() {
    logMessage("=== Audio System Status ===");
    
    if (!g_audioManager) {
        logMessage("Audio System: Not initialized");
        return;
    }
    
    logMessage("Audio System: " + std::string(g_audioManager->isRunning() ? "Running" : "Stopped"));
    logMessage("Processing: " + std::string(g_audioManager->isProcessing() ? "Active" : "Inactive"));
    logMessage("Current Volume: " + std::to_string(g_audioManager->getCurrentVolume()));
    logMessage("Peak Volume: " + std::to_string(g_audioManager->getPeakVolume()));
    logMessage("Average Volume: " + std::to_string(g_audioManager->getAverageVolume()));
    logMessage("Frequency Centroid: " + std::to_string(g_audioManager->getFrequencyCentroid()) + " Hz");
    logMessage("Samples Processed: " + std::to_string(g_audioManager->getSamplesProcessed()));
    logMessage("Processing Load: " + std::to_string(g_audioManager->getProcessingLoad() * 100.0) + "%");
    logMessage("Buffer Underruns: " + std::to_string(g_audioManager->getUnderruns()));
    logMessage("Buffer Overruns: " + std::to_string(g_audioManager->getOverruns()));
    logMessage("Audio Direction: " + g_audioManager->getAudioDirection());
    
    auto settings = g_audioManager->getSettings();
    logMessage("Sample Rate: " + std::to_string(settings.sample_rate) + " Hz");
    logMessage("Buffer Size: " + std::to_string(settings.buffer_size));
    logMessage("Channels: " + std::to_string(settings.channels));
    logMessage("Processing Enabled: " + std::string(settings.enable_processing ? "Yes" : "No"));
    logMessage("Noise Suppression: " + std::string(settings.enable_noise_suppression ? "Yes" : "No"));
    logMessage("3D Audio: " + std::string(settings.enable_3d_audio ? "Yes" : "No"));
    logMessage("Equalizer: " + std::string(settings.enable_equalizer ? "Yes" : "No"));
    logMessage("Compressor: " + std::string(settings.enable_compressor ? "Yes" : "No"));
    logMessage("Crossfeed: " + std::string(settings.enable_crossfeed ? "Yes" : "No"));
    
    // Show equalizer status
    if (settings.enable_equalizer) {
        logMessage("\n=== Equalizer Status ===");
        size_t bandCount = g_audioManager->getEqualizerBandCount();
        std::vector<std::string> bandNames = {"60Hz", "200Hz", "1kHz", "5kHz", "15kHz"};
        
        for (size_t i = 0; i < bandCount && i < bandNames.size(); ++i) {
            double gain = g_audioManager->getEqualizerBandGain(i);
            logMessage("Band " + std::to_string(i) + " (" + bandNames[i] + "): " + std::to_string(gain));
        }
    }
}

void testAudioSystem() {
    logMessage("🧪 Testing Audio System...");
    
    if (!g_audioManager || !g_audioManager->isRunning()) {
        logMessage("❌ Audio system not running");
        return;
    }
    
    // Generate test audio data
    std::vector<float> leftChannel(512);
    std::vector<float> rightChannel(512);
    
    // Generate sine wave test signal
    double frequency = 440.0; // A4 note
    double sampleRate = 44100.0;
    
    for (size_t i = 0; i < leftChannel.size(); ++i) {
        double t = static_cast<double>(i) / sampleRate;
        float sample = static_cast<float>(std::sin(2.0 * M_PI * frequency * t) * 0.1);
        leftChannel[i] = sample;
        rightChannel[i] = sample * 0.8f; // Slightly different for stereo effect
    }
    
    // Inject test data
    g_audioManager->injectAudioData(leftChannel, rightChannel);
    
    // Wait for processing
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    double currentVolume = g_audioManager->getCurrentVolume();
    if (currentVolume > 0.01) {
        logMessage("✅ Audio processing test successful (volume: " + std::to_string(currentVolume) + ")");
    } else {
        logMessage("⚠️ Audio processing test - low volume detected");
    }
    
    // Test frequency analysis
    auto spectrum = g_audioManager->getFrequencySpectrum();
    double spectralEnergy = 0.0;
    for (double bin : spectrum) {
        spectralEnergy += bin;
    }
    
    if (spectralEnergy > 0.01) {
        logMessage("✅ Frequency analysis test successful");
    } else {
        logMessage("⚠️ Frequency analysis test - low spectral energy");
    }
    
    logMessage("Audio Direction: " + g_audioManager->getAudioDirection());
    logMessage("Stereo Width: " + std::to_string(g_audioManager->getStereoWidth()));
    
    // Test equalizer
    logMessage("Testing equalizer presets...");
    g_audioManager->setEqualizerPreset("bass_boost");
    logMessage("Applied bass boost preset");
    
    g_audioManager->setEqualizerPreset("flat");
    logMessage("Applied flat preset");
}

// =============================================================================
// AUDIO CONFIGURATION HELPERS
// =============================================================================
AudioSettings createOptimizedAudioSettings() {
    AudioSettings settings;
    
    // Optimized for gaming
    settings.enable_processing = true;
    settings.enable_directional_audio = true;
    settings.enable_noise_suppression = true;
    settings.enable_bass_boost = true;
    settings.enable_treble_boost = false;
    settings.enable_compressor = true;
    settings.enable_3d_audio = true;
    settings.enable_equalizer = true;
    settings.enable_limiter = true;
    settings.enable_crossfeed = false; // Usually disabled for gaming
    
    settings.volume_threshold = 0.03;
    settings.frequency_threshold = 1500.0;
    settings.gain = 1.2;
    settings.compression_ratio = 3.0;
    settings.stereo_width = 1.5;
    settings.limiter_threshold = 0.95;
    
    return settings;
}

AudioSettings createBalancedAudioSettings() {
    AudioSettings settings;
    
    // Balanced for general use
    settings.enable_processing = true;
    settings.enable_directional_audio = true;
    settings.enable_noise_suppression = true;
    settings.enable_bass_boost = false;
    settings.enable_treble_boost = false;
    settings.enable_compressor = false;
    settings.enable_3d_audio = false;
    settings.enable_equalizer = false;
    settings.enable_limiter = true;
    settings.enable_crossfeed = false;
    
    settings.volume_threshold = 0.05;
    settings.gain = 1.0;
    settings.stereo_width = 1.0;
    settings.limiter_threshold = 0.98;
    
    return settings;
}

AudioSettings createAudiophileSettings() {
    AudioSettings settings;
    
    // High-quality for music listening
    settings.enable_processing = true;
    settings.enable_directional_audio = false;
    settings.enable_noise_suppression = false;
    settings.enable_bass_boost = false;
    settings.enable_treble_boost = false;
    settings.enable_compressor = false;
    settings.enable_3d_audio = false;
    settings.enable_equalizer = true;
    settings.enable_limiter = true;
    settings.enable_crossfeed = true; // Good for headphones
    
    settings.volume_threshold = 0.001; // Very low threshold
    settings.gain = 1.0;
    settings.stereo_width = 1.0;
    settings.limiter_threshold = 0.99;
    settings.crossfeed_amount = 0.2;
    
    return settings;
}

void applyAudioPreset(const std::string& presetName) {
    if (!g_audioManager) return;
    
    AudioSettings settings;
    
    if (presetName == "gaming") {
        settings = createOptimizedAudioSettings();
        g_audioManager->updateSettings(settings);
        g_audioManager->setEqualizerPreset("v_shape");
        logMessage("Applied gaming audio preset with V-shaped EQ");
    } else if (presetName == "balanced") {
        settings = createBalancedAudioSettings();
        g_audioManager->updateSettings(settings);
        g_audioManager->setEqualizerPreset("flat");
        logMessage("Applied balanced audio preset with flat EQ");
    } else if (presetName == "audiophile") {
        settings = createAudiophileSettings();
        g_audioManager->updateSettings(settings);
        g_audioManager->setEqualizerPreset("flat");
        logMessage("Applied audiophile audio preset with flat EQ");
    } else {
        logMessage("Unknown audio preset: " + presetName);
        return;
    }
}

// =============================================================================
// ADVANCED AUDIO PROCESSING FUNCTIONS
// =============================================================================
void generateTestTones() {
    if (!g_audioManager || !g_audioManager->isRunning()) {
        logMessage("❌ Audio system not available for test tone generation");
        return;
    }
    
    logMessage("🎵 Generating test tones...");
    
    // Test frequencies: 220Hz, 440Hz, 880Hz, 1760Hz
    std::vector<double> testFrequencies = {220.0, 440.0, 880.0, 1760.0};
    double sampleRate = 44100.0;
    size_t bufferSize = 512;
    
    for (double frequency : testFrequencies) {
        std::vector<float> leftChannel(bufferSize);
        std::vector<float> rightChannel(bufferSize);
        
        for (size_t i = 0; i < bufferSize; ++i) {
            double t = static_cast<double>(i) / sampleRate;
            float sample = static_cast<float>(std::sin(2.0 * M_PI * frequency * t) * 0.2);
            leftChannel[i] = sample;
            rightChannel[i] = sample;
        }
        
        g_audioManager->injectAudioData(leftChannel, rightChannel);
        
        logMessage("Generated " + std::to_string(frequency) + "Hz test tone");
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    
    logMessage("✅ Test tone generation complete");
}

void calibrateAudioLevels() {
    if (!g_audioManager || !g_audioManager->isRunning()) {
        logMessage("❌ Audio system not available for calibration");
        return;
    }
    
    logMessage("🔧 Calibrating audio levels...");
    
    // Reset statistics
    g_audioManager->resetStats();
    
    // Generate calibration signal
    generateTestTones();
    
    // Wait for processing
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // Get measured levels
    double peakLevel = g_audioManager->getPeakVolume();
    double avgLevel = g_audioManager->getAverageVolume();
    double freqCentroid = g_audioManager->getFrequencyCentroid();
    
    logMessage("Calibration results:");
    logMessage("  Peak Level: " + std::to_string(peakLevel));
    logMessage("  Average Level: " + std::to_string(avgLevel));
    logMessage("  Frequency Centroid: " + std::to_string(freqCentroid) + " Hz");
    
    // Adjust settings if needed
    auto settings = g_audioManager->getSettings();
    if (peakLevel > 0.8) {
        settings.gain *= 0.8;
        logMessage("Reduced gain to prevent clipping");
    } else if (peakLevel < 0.1) {
        settings.gain *= 1.5;
        logMessage("Increased gain for better signal level");
    }
    
    g_audioManager->updateSettings(settings);
    
    logMessage("✅ Audio calibration complete");
}

void performAudioBenchmark() {
    if (!g_audioManager || !g_audioManager->isRunning()) {
        logMessage("❌ Audio system not available for benchmark");
        return;
    }
    
    logMessage("⚡ Performing audio processing benchmark...");
    
    g_audioManager->resetStats();
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    // Generate continuous test signal for 2 seconds
    const size_t totalFrames = 200;
    const size_t bufferSize = 512;
    
    for (size_t frame = 0; frame < totalFrames; ++frame) {
        std::vector<float> leftChannel(bufferSize);
        std::vector<float> rightChannel(bufferSize);
        
        // Generate complex test signal with multiple frequencies
        double sampleRate = 44100.0;
        for (size_t i = 0; i < bufferSize; ++i) {
            double t = static_cast<double>(frame * bufferSize + i) / sampleRate;
            
            // Mix of multiple frequencies
            double signal = 0.0;
            signal += 0.3 * std::sin(2.0 * M_PI * 220.0 * t);  // Bass
            signal += 0.4 * std::sin(2.0 * M_PI * 440.0 * t);  // Mid
            signal += 0.2 * std::sin(2.0 * M_PI * 1760.0 * t); // Treble
            signal += 0.1 * std::sin(2.0 * M_PI * 8000.0 * t); // High freq
            
            leftChannel[i] = static_cast<float>(signal * 0.5);
            rightChannel[i] = static_cast<float>(signal * 0.4); // Slight stereo difference
        }
        
        g_audioManager->injectAudioData(leftChannel, rightChannel);
        
        // Brief pause to simulate real-time processing
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }
    
    // Wait for all processing to complete
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto totalTime = std::chrono::duration<double>(endTime - startTime).count();
    
    // Get performance metrics
    uint64_t samplesProcessed = g_audioManager->getSamplesProcessed();
    double averageLoad = g_audioManager->getProcessingLoad();
    uint64_t underruns = g_audioManager->getUnderruns();
    uint64_t overruns = g_audioManager->getOverruns();
    
    logMessage("Benchmark Results:");
    logMessage("  Total Time: " + std::to_string(totalTime) + " seconds");
    logMessage("  Samples Processed: " + std::to_string(samplesProcessed));
    logMessage("  Processing Rate: " + std::to_string(samplesProcessed / totalTime) + " samples/sec");
    logMessage("  Average CPU Load: " + std::to_string(averageLoad * 100.0) + "%");
    logMessage("  Buffer Underruns: " + std::to_string(underruns));
    logMessage("  Buffer Overruns: " + std::to_string(overruns));
    
    if (averageLoad < 0.5 && underruns == 0 && overruns == 0) {
        logMessage("✅ Excellent performance - system can handle real-time audio");
    } else if (averageLoad < 0.8 && underruns < 5 && overruns < 5) {
        logMessage("✅ Good performance - suitable for most applications");
    } else {
        logMessage("⚠️ Performance issues detected - consider optimization");
    }
    
    logMessage("✅ Audio benchmark complete");
}