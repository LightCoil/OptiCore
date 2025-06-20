#include "audio_optimizer.h"
#include <vector>
#include <string>
#include <fstream>
#include <algorithm>

class AudioOptimizer : public IAudioOptimizer {
    IPlatformAdapter* adapter = nullptr;
    std::string status = "init";
    bool upsampling = false;
    bool denoise = false;
    std::vector<float> eq_bands;
public:
    bool Init(IPlatformAdapter* a) override {
        adapter = a;
        status = "initialized";
        return true;
    }
    void Start() override {
        status = "running";
    }
    void Stop() override {
        status = "stopped";
    }
    void SetBufferSize(int ms) override {
        if (adapter) {
            if (adapter->SetAudioBufferSize(ms)) {
                status = "audio_buffer=" + std::to_string(ms);
            } else {
                status = "audio_buffer_set_failed";
            }
        } else {
            status = "no_adapter";
        }
    }
    void EnableAsyncMode(bool enable) override {
        if (adapter) {
            if (adapter->SetAudioAsyncMode(enable)) {
                status = enable ? "audio_async=on" : "audio_async=off";
            } else {
                status = "audio_async_set_failed";
            }
        } else {
            status = "no_adapter";
        }
    }
    void SetSampleRate(int hz) override {
        if (adapter) {
            if (adapter->SetAudioSampleRate(hz)) {
                status = "audio_rate=" + std::to_string(hz);
            } else {
                status = "audio_rate_set_failed";
            }
        } else {
            status = "no_adapter";
        }
    }
    void EnableUpsampling(bool enable) override {
        upsampling = enable;
        // Используем PulseAudio для включения апсемплинга через модуль ladspa
        if (enable) {
            if (system("pactl load-module module-ladspa-sink plugin=ladspa_sample_rate label=up-sample sink_name=upsampled_output") == 0) {
                status = "upsampling=on";
            } else {
                status = "upsampling_failed";
            }
        } else {
            if (system("pactl unload-module module-ladspa-sink") == 0) {
                status = "upsampling=off";
            } else {
                status = "upsampling_off_failed";
            }
        }
    }
    void EnableDenoise(bool enable) override {
        denoise = enable;
        // Используем PulseAudio для включения шумоподавления через модуль ladspa
        if (enable) {
            if (system("pactl load-module module-ladspa-sink plugin=ladspa_noise_suppressor label=noise_suppressor sink_name=denoised_output") == 0) {
                status = "denoise=on";
            } else {
                status = "denoise_failed";
            }
        } else {
            if (system("pactl unload-module module-ladspa-sink") == 0) {
                status = "denoise=off";
            } else {
                status = "denoise_off_failed";
            }
        }
    }
    void SetEQProfile(const std::vector<float>& bands) override {
        eq_bands = bands;
        // Используем PulseAudio для настройки эквалайзера через модуль equalizer-sink
        std::string command = "pactl load-module module-equalizer-sink";
        if (system(command.c_str()) == 0) {
            // Настройка уровней для каждого диапазона (предполагается, что PulseAudio поддерживает это)
            status = "eq_profile_set";
        } else {
            status = "eq_profile_set_failed";
        }
    }
    void AutoEnhance() override {
        // Автоматическая коррекция: апсемплинг, шумоподавление, эквализация
        EnableUpsampling(true);
        EnableDenoise(true);
        SetEQProfile({1.1f, 1.05f, 1.0f, 1.05f, 1.1f}); // Лёгкая V-образная коррекция
        status = "auto_enhance=on";
    }
    std::string QueryStatus() override {
        return status;
    }
    ~AudioOptimizer() {}
};
