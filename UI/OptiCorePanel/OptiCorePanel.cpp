#include "OptiCorePanel.h"
#include <imgui.h>
#include <imgui_internal.h>
#include <array>
#include <chrono>
#include <fstream>
#include <sstream>
#include <string>
#include <cstring>
#include <thread>
#include <algorithm>

// NASA/Hi-Tech стиль: цветовая схема, шрифты, иконки
static void SetOptiCoreDarkTheme() {
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;
    colors[ImGuiCol_WindowBg]         = ImVec4(0.07f, 0.09f, 0.13f, 1.00f);
    colors[ImGuiCol_Tab]              = ImVec4(0.13f, 0.16f, 0.22f, 0.86f);
    colors[ImGuiCol_TabActive]        = ImVec4(0.18f, 0.22f, 0.32f, 1.00f);
    colors[ImGuiCol_TabHovered]       = ImVec4(0.22f, 0.28f, 0.40f, 1.00f);
    colors[ImGuiCol_Button]           = ImVec4(0.18f, 0.22f, 0.32f, 1.00f);
    colors[ImGuiCol_ButtonHovered]    = ImVec4(0.22f, 0.28f, 0.40f, 1.00f);
    colors[ImGuiCol_ButtonActive]     = ImVec4(0.30f, 0.36f, 0.50f, 1.00f);
    colors[ImGuiCol_FrameBg]          = ImVec4(0.13f, 0.16f, 0.22f, 1.00f);
    colors[ImGuiCol_FrameBgHovered]   = ImVec4(0.18f, 0.22f, 0.32f, 1.00f);
    colors[ImGuiCol_FrameBgActive]    = ImVec4(0.22f, 0.28f, 0.40f, 1.00f);
    style.WindowRounding = 6.0f;
    style.FrameRounding = 4.0f;
    style.TabRounding = 4.0f;
    style.GrabRounding = 4.0f;
    style.ScrollbarRounding = 6.0f;
}

// NASA-иконки (можно заменить на кастомные)
static const char* tab_icons[] = {"\xef\x8c\x9b", "\xef\x95\x81", "\xef\x8c\x9a", "\xef\x8c\x82", "\xef\x8c\x8a", "\xef\x8c\x8b", "\xef\x8c\x90", "\xef\x8c\x92", "\xef\x8c\x93", "\xef\x8c\x94"};
static const char* tab_names[] = {"CPU", "Memory", "GPU", "Audio", "Power", "IO", "Аналитика", "Профили", "Сценарии", "Логи"};

// --- Метрики CPU ---
static float GetCpuUsage() {
    static long long last_total = 0, last_idle = 0;
    std::ifstream stat("/proc/stat");
    std::string line;
    std::getline(stat, line);
    std::istringstream iss(line);
    std::string cpu;
    long long user, nice, system, idle, iowait, irq, softirq, steal;
    iss >> cpu >> user >> nice >> system >> idle >> iowait >> irq >> softirq >> steal;
    long long idle_all = idle + iowait;
    long long non_idle = user + nice + system + irq + softirq + steal;
    long long total = idle_all + non_idle;
    float usage = 0.0f;
    if (last_total != 0) {
        long long totald = total - last_total;
        long long idled = idle_all - last_idle;
        usage = (float)(totald - idled) / (float)totald * 100.0f;
    }
    last_total = total;
    last_idle = idle_all;
    return usage;
}

// --- Метрики памяти ---
static float GetMemUsage() {
    std::ifstream meminfo("/proc/meminfo");
    std::string line;
    long long total = 0, available = 0;
    while (std::getline(meminfo, line)) {
        if (line.find("MemTotal:") == 0) {
            sscanf(line.c_str(), "MemTotal: %lld", &total);
        } else if (line.find("MemAvailable:") == 0) {
            sscanf(line.c_str(), "MemAvailable: %lld", &available);
        }
        if (total && available) break;
    }
    if (total == 0) return 0.0f;
    return (float)(total - available) / (float)total * 100.0f;
}

// --- Метрики GPU (AMD/Intel через sysfs, NVIDIA через nvidia-smi) ---
static float GetGpuUsage() {
    std::ifstream f("/sys/class/drm/card0/device/gpu_busy_percent");
    float val = 0;
    if (f) f >> val;
    else {
        // Альтернатива: NVIDIA через nvidia-smi
        FILE* pipe = popen("nvidia-smi --query-gpu=utilization.gpu --format=csv,noheader,nounits", "r");
        if (pipe) {
            char buf[32];
            if (fgets(buf, sizeof(buf), pipe)) val = atof(buf);
            pclose(pipe);
        }
    }
    return std::clamp(val, 0.0f, 100.0f);
}

// --- Метрики энергопотребления (Power) ---
static float GetPowerUsage() {
    std::ifstream f("/sys/class/power_supply/BAT0/power_now");
    float val = 0;
    if (f) { int mw = 0; f >> mw; val = mw / 1000.0f; }
    else {
        std::ifstream f2("/sys/class/hwmon/hwmon0/power1_average");
        if (f2) { int uw = 0; f2 >> uw; val = uw / 1000000.0f; }
    }
    return val;
}

// --- Метрики IO (операции в секунду) ---
static float GetIOOps() {
    static long long last_rd = 0, last_wr = 0;
    std::ifstream f("/proc/diskstats");
    std::string line;
    long long rd = 0, wr = 0;
    while (std::getline(f, line)) {
        if (line.find(" sda ") != std::string::npos || line.find(" nvme0n1 ") != std::string::npos) {
            int major, minor;
            char dev[32];
            long long reads, rd_merged, rd_sectors, rd_time, writes, wr_merged, wr_sectors, wr_time;
            sscanf(line.c_str(), "%d %d %s %lld %lld %lld %lld %lld %lld %lld %lld", &major, &minor, dev, &reads, &rd_merged, &rd_sectors, &rd_time, &writes, &wr_merged, &wr_sectors, &wr_time);
            rd += reads;
            wr += writes;
        }
    }
    float ops = 0.0f;
    if (last_rd != 0 || last_wr != 0) {
        ops = (float)((rd - last_rd) + (wr - last_wr));
    }
    last_rd = rd;
    last_wr = wr;
    return ops;
}

// --- Метрики Audio (качество: sample rate, async, буфер) ---
static float GetAudioQuality() {
    // Для примера: если async включён — 100, иначе 90
    // Можно расширить через PulseAudio/ALSA API
    return 95.0f;
}

OptiCorePanel::OptiCorePanel(std::shared_ptr<ICpuOptimizer> cpu,
                             std::shared_ptr<IMemoryOptimizer> mem,
                             std::shared_ptr<IGPUOptimizer> gpu,
                             std::shared_ptr<IAudioOptimizer> audio,
                             std::shared_ptr<IPowerManager> power,
                             std::shared_ptr<IIOOptimizer> io)
    : cpu_(cpu), mem_(mem), gpu_(gpu), audio_(audio), power_(power), io_(io) {
    SetOptiCoreDarkTheme();
}

void OptiCorePanel::Update() {
    static auto last_update = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();
    if (std::chrono::duration_cast<std::chrono::milliseconds>(now - last_update).count() < 500) return;
    last_update = now;
    cpu_status_ = cpu_ ? cpu_->QueryStatus() : "N/A";
    mem_status_ = mem_ ? mem_->QueryStatus() : "N/A";
    gpu_status_ = gpu_ ? gpu_->QueryStatus() : "N/A";
    audio_status_ = audio_ ? audio_->QueryStatus() : "N/A";
    power_status_ = power_ ? power_->QueryStatus() : "N/A";
    io_status_ = io_ ? io_->QueryStatus() : "N/A";
    // --- обновление метрик ---
    static float cpu_usage[60] = {};
    static float mem_usage[60] = {};
    static float gpu_usage[60] = {};
    static float power_usage[60] = {};
    static float io_ops[60] = {};
    static float audio_quality[60] = {};
    memmove(cpu_usage, cpu_usage + 1, sizeof(cpu_usage) - sizeof(float));
    memmove(mem_usage, mem_usage + 1, sizeof(mem_usage) - sizeof(float));
    memmove(gpu_usage, gpu_usage + 1, sizeof(gpu_usage) - sizeof(float));
    memmove(power_usage, power_usage + 1, sizeof(power_usage) - sizeof(float));
    memmove(io_ops, io_ops + 1, sizeof(io_ops) - sizeof(float));
    memmove(audio_quality, audio_quality + 1, sizeof(audio_quality) - sizeof(float));
    cpu_usage[59] = GetCpuUsage();
    mem_usage[59] = GetMemUsage();
    gpu_usage[59] = GetGpuUsage();
    power_usage[59] = GetPowerUsage();
    io_ops[59] = GetIOOps();
    audio_quality[59] = GetAudioQuality();
    memcpy(this->cpu_usage_, cpu_usage, sizeof(cpu_usage));
    memcpy(this->mem_usage_, mem_usage, sizeof(mem_usage));
    memcpy(this->gpu_usage_, gpu_usage, sizeof(gpu_usage));
    memcpy(this->power_usage_, power_usage, sizeof(power_usage));
    memcpy(this->io_ops_, io_ops, sizeof(io_ops));
    memcpy(this->audio_quality_, audio_quality, sizeof(audio_quality));
}

void OptiCorePanel::ShowAnalyticsTab() {
    ImGui::Text("\xef\x8c\x90 Аналитика эффективности");
    if (ImGui::Button("Запустить анализ")) {
        // Пример: сравнить до/после для CPU
        AnalyticsEntry entry;
        entry.metric = "CPU";
        entry.before = cpu_usage_[0];
        entry.after = cpu_usage_[59];
        entry.efficiency = entry.before > 0 ? (entry.before - entry.after) / entry.before * 100.0f : 0.0f;
        entry.timestamp = "now"; // TODO: timestamp
        analytics_history_.push_back(entry);
        AddLog("Выполнен анализ эффективности CPU");
    }
    ImGui::Separator();
    ImGui::Columns(4, nullptr, false);
    ImGui::Text("Метрика"); ImGui::NextColumn();
    ImGui::Text("До"); ImGui::NextColumn();
    ImGui::Text("После"); ImGui::NextColumn();
    ImGui::Text("Эффективность, %"); ImGui::NextColumn();
    ImGui::Separator();
    for (const auto& e : analytics_history_) {
        ImGui::Text("%s", e.metric.c_str()); ImGui::NextColumn();
        ImGui::Text("%.1f", e.before); ImGui::NextColumn();
        ImGui::Text("%.1f", e.after); ImGui::NextColumn();
        ImGui::Text("%.1f", e.efficiency); ImGui::NextColumn();
    }
    ImGui::Columns(1);
}

void OptiCorePanel::ShowProfilesTab() {
    ImGui::Text("\xef\x8c\x92 Профили оптимизации");
    if (ImGui::Button("Создать профиль")) {
        profiles_.push_back({"Новый профиль", "Описание"});
        AddLog("Создан новый профиль");
    }
    ImGui::Separator();
    for (size_t i = 0; i < profiles_.size(); ++i) {
        ImGui::PushID((int)i);
        bool selected = (int)i == active_profile_;
        if (ImGui::Selectable(profiles_[i].name.c_str(), selected)) {
            active_profile_ = (int)i;
            AddLog("Активирован профиль: " + profiles_[i].name);
        }
        ImGui::SameLine();
        ImGui::Text("%s", profiles_[i].description.c_str());
        ImGui::PopID();
    }
}

void OptiCorePanel::ShowScenariosTab() {
    ImGui::Text("\xef\x8c\x93 Сценарии");
    if (ImGui::Button("Добавить сценарий")) {
        scenarios_.push_back({"Новый сценарий", "Описание", {}});
        AddLog("Добавлен новый сценарий");
    }
    ImGui::Separator();
    for (const auto& s : scenarios_) {
        ImGui::Text("%s: %s", s.name.c_str(), s.description.c_str());
        for (const auto& act : s.actions) {
            ImGui::BulletText("%s", act.c_str());
        }
    }
}

void OptiCorePanel::ShowLogsTab() {
    ImGui::Text("\xef\x8c\x94 Логи событий");
    ImGui::Separator();
    ImGui::BeginChild("LogChild", ImVec2(0, 300), true);
    for (const auto& log : logs_) {
        ImGui::TextUnformatted(log.c_str());
    }
    ImGui::EndChild();
}

void OptiCorePanel::AddLog(const std::string& msg) {
    if (logs_.size() > 1000) logs_.erase(logs_.begin());
    logs_.push_back(msg);
}

void OptiCorePanel::Render() {
    ImGui::SetNextWindowSize(ImVec2(900, 600), ImGuiCond_FirstUseEver);
    ImGui::Begin("OptiCorePanel — Центр управления оптимизацией", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize);
    if (ImGui::BeginTabBar("OptiCoreTabs", ImGuiTabBarFlags_Reorderable)) {
        for (int i = 0; i < 10; ++i) {
            char label[64];
            snprintf(label, sizeof(label), "%s %s", tab_icons[i], tab_names[i]);
            if (ImGui::BeginTabItem(label)) {
                if (i == 0) { // CPU
                    ImGui::Text("Статус: %s", cpu_status_.c_str());
                    ImGui::Spacing();
                    if (ImGui::Button("Performance")) cpu_->SetPerformanceMode();
                    ImGui::SameLine();
                    if (ImGui::Button("Balanced")) cpu_->SetBalancedMode();
                    ImGui::SameLine();
                    if (ImGui::Button("PowerSave")) cpu_->SetPowerSaveMode();
                    ImGui::Separator();
                    ImGui::Text("\xef\x87\xb1 Загрузка CPU");
                    ImGui::PlotLines("", cpu_usage_, 60, 0, nullptr, 0.0f, 100.0f, ImVec2(0, 60));
                } else if (i == 1) { // Memory
                    ImGui::Text("Статус: %s", mem_status_.c_str());
                    if (ImGui::Button("Drop Caches")) mem_->DropCaches();
                    ImGui::SameLine();
                    if (ImGui::Button("Enable HugePages")) mem_->EnableHugePages(true);
                    ImGui::SameLine();
                    if (ImGui::Button("Disable HugePages")) mem_->EnableHugePages(false);
                    ImGui::Separator();
                    ImGui::Text("\xef\x8c\x81 Использование памяти");
                    ImGui::PlotLines("", mem_usage_, 60, 0, nullptr, 0.0f, 100.0f, ImVec2(0, 60));
                } else if (i == 2) { // GPU
                    ImGui::Text("Статус: %s", gpu_status_.c_str());
                    if (ImGui::Button("Performance")) gpu_->SetPerformanceLevel("high");
                    ImGui::SameLine();
                    if (ImGui::Button("PowerSave")) gpu_->SetPerformanceLevel("low");
                    ImGui::Separator();
                    ImGui::Text("\xef\x8c\x9a Загрузка GPU");
                    ImGui::PlotLines("", gpu_usage_, 60, 0, nullptr, 0.0f, 100.0f, ImVec2(0, 60));
                } else if (i == 3) { // Audio
                    ImGui::Text("Статус: %s", audio_status_.c_str());
                    if (ImGui::Button("AutoEnhance")) audio_->AutoEnhance();
                    ImGui::SameLine();
                    if (ImGui::Button("Async On")) audio_->EnableAsyncMode(true);
                    ImGui::SameLine();
                    if (ImGui::Button("Async Off")) audio_->EnableAsyncMode(false);
                    ImGui::Separator();
                    ImGui::Text("\xef\x8c\x82 Качество аудио");
                    ImGui::PlotLines("", audio_quality_, 60, 0, nullptr, 80.0f, 100.0f, ImVec2(0, 60));
                } else if (i == 4) { // Power
                    ImGui::Text("Статус: %s", power_status_.c_str());
                    if (ImGui::Button("Performance")) power_->SetPowerProfile("performance");
                    ImGui::SameLine();
                    if (ImGui::Button("Balanced")) power_->SetPowerProfile("balanced");
                    ImGui::SameLine();
                    if (ImGui::Button("PowerSave")) power_->SetPowerProfile("powersave");
                    ImGui::Separator();
                    ImGui::Text("\xef\x8c\x8a Энергопотребление");
                    ImGui::PlotLines("", power_usage_, 60, 0, nullptr, 0.0f, 100.0f, ImVec2(0, 60));
                } else if (i == 5) { // IO
                    ImGui::Text("Статус: %s", io_status_.c_str());
                    if (ImGui::Button("Async IO On")) io_->EnableAsyncIO(true);
                    ImGui::SameLine();
                    if (ImGui::Button("Async IO Off")) io_->EnableAsyncIO(false);
                    ImGui::Separator();
                    ImGui::Text("\xef\x8c\x8b IO-операции");
                    ImGui::PlotLines("", io_ops_, 60, 0, nullptr, 0.0f, 100.0f, ImVec2(0, 60));
                } else if (i == 6) { ShowAnalyticsTab(); }
                else if (i == 7) { ShowProfilesTab(); }
                else if (i == 8) { ShowScenariosTab(); }
                else if (i == 9) { ShowLogsTab(); }
                ImGui::EndTabItem();
            }
        }
        ImGui::EndTabBar();
    }
    ImGui::End();
} 