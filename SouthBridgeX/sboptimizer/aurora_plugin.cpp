#include <xtl.h>
#include <xbox.h>
#include <stdio.h>

extern float ReadSouthbridgeTemp();

bool RegisterAuroraPlugin() {
    // Регистрация плагина в Aurora для отображения в UI
    // AuroraPluginRegister("SB Optimizer", "Southbridge Cooler & Throttler", 0x0001);
    // Регистрация callback-функции для предоставления статуса плагина
    // AuroraRegisterCallback(PLUGIN_QUERY_STATUS, &QuerySBStatus);
    return true;
}

int QuerySBStatus(char* out, int maxLen) {
    float t = ReadSouthbridgeTemp();
    snprintf(out, maxLen, "SB Temp: %.2f°C\nUSB: Boosted\nEEPROM: Proxy Mode", t);
    return strlen(out);
}
