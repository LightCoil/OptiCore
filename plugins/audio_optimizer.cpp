#include "audio_optimizer.h"

class AudioOptimizer : public IAudioOptimizer {
    IPlatformAdapter* adapter = nullptr;
    std::string status = "init";
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
        if (adapter) adapter->SetAudioBufferSize(ms);
        status = "audio_buffer=" + std::to_string(ms);
    }
    void EnableAsyncMode(bool enable) override {
        if (adapter) adapter->SetAudioAsyncMode(enable);
        status = enable ? "audio_async=on" : "audio_async=off";
    }
    void SetSampleRate(int hz) override {
        if (adapter) adapter->SetAudioSampleRate(hz);
        status = "audio_rate=" + std::to_string(hz);
    }
    std::string QueryStatus() override {
        return status;
    }
    ~AudioOptimizer() {}
}; 