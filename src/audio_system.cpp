// description: Implementation of the audio analysis system using PortAudio and FFTW.
// developer: ingekastel
// license: GNU General Public License v3.0
// version: 2.7.0
// date: 2025-06-26
// project: Tactical Aim Assist

#include "audio_system.h"
#include "globals.h"
#include "gui.h"
#include <iostream>
#include <cmath>
#include <numeric>

// FIX: Constructor implementation now matches the header declaration (2 arguments).
AudioManager::AudioManager(uint32_t sample_rate, uint32_t buffer_size)
    : m_sample_rate(sample_rate), m_buffer_size(buffer_size),
      m_ring_buffer(sample_rate * 2)
{
    Pa_Initialize();

    m_fft_in.resize(m_buffer_size);
    m_fft_out.resize(m_buffer_size / 2 + 1);

    m_fft_plan = fftwf_plan_dft_r2c_1d(
        m_buffer_size,
        m_fft_in.data(),
        reinterpret_cast<fftwf_complex*>(m_fft_out.data()),
        FFTW_MEASURE
    );
}

AudioManager::~AudioManager() {
    stop();
    fftwf_destroy_plan(m_fft_plan);
    Pa_Terminate();
    delete m_last_alert.load();
}

// ... (start, stop, getLatestAlert, and paCallback functions remain unchanged) ...
bool AudioManager::start() {
    m_running = true;

    const PaDeviceIndex dev_idx = Pa_GetDefaultOutputDevice();
    const PaDeviceInfo* dev_info = Pa_GetDeviceInfo(dev_idx);

    PaStreamParameters stream_params = {};
    stream_params.device = dev_idx;
    stream_params.channelCount = dev_info->maxInputChannels > 0 ? dev_info->maxInputChannels : 1;
    stream_params.sampleFormat = paFloat32;
    stream_params.suggestedLatency = dev_info->defaultLowInputLatency;

    PaError err = Pa_OpenStream(
        &m_stream, &stream_params, nullptr, m_sample_rate, m_buffer_size,
        paClipOff, paCallback, this
    );

    if (err != paNoError) {
        logMessage("PortAudio Error: Failed to open stream.");
        return false;
    }

    err = Pa_StartStream(m_stream);
    if (err != paNoError) {
        logMessage("PortAudio Error: Failed to start stream.");
        return false;
    }

    m_analysis_thread = std::thread(&AudioManager::analysisLoop, this);
    logMessage("Audio analysis system (FFTW) started.");
    return true;
}

void AudioManager::stop() {
    if (m_running.exchange(false)) {
        if (m_analysis_thread.joinable()) m_analysis_thread.join();
        if (m_stream && Pa_IsStreamActive(m_stream) > 0) {
            Pa_StopStream(m_stream);
            Pa_CloseStream(m_stream);
        }
        logMessage("Audio analysis system stopped.");
    }
}

std::string AudioManager::getLatestAlert() {
    std::string* alert_ptr = m_last_alert.exchange(nullptr);
    if (alert_ptr) {
        std::string alert = *alert_ptr;
        delete alert_ptr;
        return alert;
    }
    return "";
}

int AudioManager::paCallback(const void* inputBuffer, void*, unsigned long framesPerBuffer,
                           const PaStreamCallbackTimeInfo*, PaStreamCallbackFlags, void* userData) {
    auto* self = static_cast<AudioManager*>(userData);
    if (self && self->m_running) {
        self->m_ring_buffer.write(static_cast<const float*>(inputBuffer), framesPerBuffer);
    }
    return paContinue;
}


void AudioManager::analysisLoop() {
    std::vector<float> chunk(m_buffer_size);
    while (m_running) {
        if (m_ring_buffer.size() >= m_buffer_size) {
            m_ring_buffer.read(chunk.data(), m_buffer_size);
            detectOnset(chunk);
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
}

void AudioManager::detectOnset(const std::vector<float>& audio_chunk) {
    std::copy(audio_chunk.begin(), audio_chunk.end(), m_fft_in.begin());

    fftwf_execute(m_fft_plan);

    float current_energy = 0.0f;
    // FIX: Access the members of our Complex struct.
    for(const auto& complex_val : m_fft_out) {
        current_energy += std::sqrt(complex_val.real * complex_val.real + complex_val.imag * complex_val.imag);
    }

    if (current_energy > m_previous_energy * m_onset_threshold && m_previous_energy > 0.01) {
        std::string alert_text = "¡Evento de Sonido! Energía: " + std::to_string(current_energy);
        std::string* new_alert = new std::string(alert_text);
        delete m_last_alert.exchange(new_alert);
    }

    m_previous_energy = current_energy;
}