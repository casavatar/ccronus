// description: Implementation of the audio analysis system.
// This file contains the logic for capturing system audio via PortAudio,
// processing it with Aubio for onset detection, and managing the
// communication between threads.
// developer: ingekastel
// license: GNU General Public License v3.0
// version: 1.2.0
// date: 2025-06-26
// project: Tactical Aim Assist

#include "audio_system.h"
#include "globals.h"
#include "gui.h" // For updating the GUI

#include <portaudio.h>
#include <aubio/aubio.h>
#include <iostream>

AudioManager::AudioManager(uint32_t sample_rate, uint32_t buffer_size, uint32_t hop_size)
    : m_sample_rate(sample_rate), m_buffer_size(buffer_size), m_hop_size(hop_size),
      m_ring_buffer(sample_rate * 5) // 5 seconds of buffer
{
    Pa_Initialize();
    // Aubio setup: "default" onset detection method. Threshold is critical for tuning.
    m_onset_obj = new_aubio_onset("default", m_buffer_size, m_hop_size, m_sample_rate);
    aubio_onset_set_threshold(m_onset_obj, 0.3f);
}

AudioManager::~AudioManager() {
    stop();
    if (m_onset_obj) {
        del_aubio_onset(m_onset_obj);
    }
    Pa_Terminate();
    delete m_last_alert.load();
}

bool AudioManager::start() {
    m_running = true;

    // Use WASAPI loopback to capture system audio on Windows
    const PaDeviceIndex dev_idx = Pa_GetDefaultOutputDevice();
    const PaDeviceInfo* dev_info = Pa_GetDeviceInfo(dev_idx);

    PaStreamParameters stream_params;
    memset(&stream_params, 0, sizeof(stream_params));
    stream_params.device = dev_idx;
    stream_params.channelCount = dev_info->maxInputChannels;
    stream_params.sampleFormat = paFloat32;
    stream_params.suggestedLatency = dev_info->defaultLowInputLatency;
    stream_params.hostApiSpecificStreamInfo = NULL;

    PaError err = Pa_OpenStream(
        &m_stream,
        &stream_params,
        nullptr, // No output
        m_sample_rate,
        m_buffer_size,
        paClipOff,
        paCallback,
        this // Pass this instance to the callback
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
    logMessage("Audio analysis system started.");
    return true;
}

void AudioManager::stop() {
    if (m_running.exchange(false)) {
        if (m_analysis_thread.joinable()) {
            m_analysis_thread.join();
        }
        if (m_stream && Pa_IsStreamActive(m_stream)) {
            Pa_StopStream(m_stream);
            Pa_CloseStream(m_stream);
            m_stream = nullptr;
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

// Static callback function for PortAudio
int AudioManager::paCallback(const void* inputBuffer, void* /*outputBuffer*/,
                           unsigned long framesPerBuffer,
                           const void* /*timeInfo*/, PaStreamCallbackFlags /*statusFlags*/,
                           void* userData) {
    AudioManager* self = static_cast<AudioManager*>(userData);
    const float* in = static_cast<const float*>(inputBuffer);

    if (self && self->m_running) {
        self->m_ring_buffer.write(in, framesPerBuffer);
    }
    return paContinue;
}

// Analysis thread main loop
void AudioManager::analysisLoop() {
    std::vector<float> analysis_buffer(m_buffer_size);
    fvec_t aubio_input = {0};
    aubio_input.length = m_hop_size;
    aubio_input.data = analysis_buffer.data();
    
    fvec_t onset_output = {0};
    onset_output.length = 1;
    float onset_value = 0;
    onset_output.data = &onset_value;

    while (m_running) {
        if (m_ring_buffer.size() >= m_hop_size) {
            m_ring_buffer.read(analysis_buffer.data(), m_hop_size);
            aubio_onset_do(m_onset_obj, &aubio_input, &onset_output);

            if (onset_value > 0.f) {
                // --- Onset Detected ---
                // This is where filtering logic for "step" vs "shot" would go.
                // For now, we generate a generic alert.
                // A real implementation would analyze the FFT, energy, etc.
                // of the `analysis_buffer` content.
                std::string* new_alert = new std::string("¡Evento de Sonido Detectado!");
                delete m_last_alert.exchange(new_alert); // Atomically replace and delete the old alert
            }
        } else {
            // Wait a bit if the buffer is empty
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
}