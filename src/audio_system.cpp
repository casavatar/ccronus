// description: Implementation of the audio analysis system using PortAudio and FFTW.
// developer: ingekastel
// license: GNU General Public License v3.0
// version: 2.8.1 (Correct PortAudio Device Iteration)
// date: 2025-07-04
// project: Tactical Aim Assist

#include "audio_system.h"
#include "globals.h"
#include "gui.h"
#include <iostream>
#include <cmath>
#include <numeric>
#include <string.h>

// ... (El constructor y destructor de AudioManager no cambian) ...
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


bool AudioManager::start() {
    m_running = true;

    // --- FIX: Correctly iterate through all devices to find the WASAPI loopback device ---
    PaDeviceIndex dev_idx = paNoDevice;
    const PaDeviceInfo* dev_info = nullptr;

    const PaHostApiIndex host_api_idx = Pa_HostApiTypeIdToHostApiIndex(paWASAPI);
    if (host_api_idx < 0) {
        logMessage("PortAudio Error: WASAPI host API not found.");
        return false;
    }

    const int device_count = Pa_GetDeviceCount();
    for (PaDeviceIndex i = 0; i < device_count; ++i) {
        const PaDeviceInfo* current_dev_info = Pa_GetDeviceInfo(i);
        if (current_dev_info &&
            current_dev_info->hostApi == host_api_idx &&
            current_dev_info->maxInputChannels > 0 &&
            strstr(current_dev_info->name, "Loopback"))
        {
            dev_idx = i;
            dev_info = current_dev_info;
            logMessage(std::string("Found WASAPI Loopback Device: ") + dev_info->name);
            break; // Found our device, exit the loop
        }
    }

    if (dev_idx == paNoDevice || dev_info == nullptr) {
        logMessage("PortAudio Error: No suitable WASAPI loopback device found.");
        logMessage("Please ensure 'Stereo Mix' or a similar recording device is enabled in Windows sound settings.");
        return false;
    }

    PaStreamParameters stream_params = {};
    stream_params.device = dev_idx;
    stream_params.channelCount = dev_info->maxInputChannels;
    stream_params.sampleFormat = paFloat32;
    stream_params.suggestedLatency = dev_info->defaultLowInputLatency;
    stream_params.hostApiSpecificStreamInfo = NULL;

    logMessage("Attempting to open stream with " + std::to_string(stream_params.channelCount) + " channels at " + std::to_string(m_sample_rate) + " Hz.");

    PaError err = Pa_OpenStream(
        &m_stream, &stream_params, nullptr, m_sample_rate, m_buffer_size,
        paClipOff, paCallback, this
    );

    if (err != paNoError) {
        std::string error_text = "PortAudio Error: Failed to open stream. Details: ";
        error_text += Pa_GetErrorText(err);
        logMessage(error_text);
        return false;
    }

    err = Pa_StartStream(m_stream);
    if (err != paNoError) {
        logMessage("PortAudio Error: Failed to start stream.");
        return false;
    }

    m_analysis_thread = std::thread(&AudioManager::analysisLoop, this);
    logMessage("Audio analysis system (FFTW) started successfully.");
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