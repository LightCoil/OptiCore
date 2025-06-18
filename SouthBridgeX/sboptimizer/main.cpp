#include <xtl.h>
#include <xbox.h>
#include <stdio.h>

extern void InitPlugin();
extern void PatchSMC();
extern void InitSBMonitor();
extern void InitUSBThrottle();
extern void InitEEPROMProxy();
extern bool RegisterAuroraPlugin();
extern "C" BOOL WINAPI DllMain(HANDLE hInstance, DWORD reason, LPVOID reserved) {
    if (reason == DLL_PROCESS_ATTACH) {
        CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)InitPlugin, NULL, 0, NULL);
    }
    return TRUE;
}

void InitPlugin() {
    PatchSMC();                     // перехватываем команды к мосту
    InitSBMonitor();                // запускаем мониторинг температуры и IRQ
    InitUSBThrottle();              // ускоряем USB
    InitEEPROMProxy();              // RAM-прокси EEPROM
    RegisterAuroraPlugin();         // подключаемся к UI Aurora
}
