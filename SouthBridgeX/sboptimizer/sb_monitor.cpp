#include <xtl.h>
#include <xbox.h>
#include <stdio.h>

float ReadSouthbridgeTemp() {
    BYTE val = 0;
    // Читаем температуру южного моста через SMC-регистр
    // Используем перехват через ReadSMC для кэширования
    val = ReadSMC(0x10); // SB Temp register
    return (float)val;
}

void SBMonitorLoop() {
    while (true) {
        float temp = ReadSouthbridgeTemp();
        if (temp > 44.0f) {
            SetFanSpeed(1, 0x60);  // повышаем скорость вентилятора для SB (fanId=1 как пример)
        } else if (temp < 38.0f) {
            SetFanSpeed(1, 0x30);  // понижаем скорость вентилятора
        } else {
            SetFanSpeed(1, 0x40);  // средняя скорость для поддержания температуры
        }
        LogSBStatus(temp);
        // Проверяем возможность перехода в режим низкого энергопотребления
        if (temp < 40.0f) {
            SendSMCLowPower(); // переводим южный мост в режим ожидания
        }
        Sleep(5000); // проверка каждые 5 секунд
    }
}

void LogSBStatus(float temp) {
    FILE* f = fopen("usb:/Aurora/Plugins/sb_monitor.log", "a");
    if (f) {
        fprintf(f, "[%lu] Temp: %.2f°C\n", GetTickCount(), temp);
        fclose(f);
    }
}

void InitSBMonitor() {
    // Запуск мониторинга в отдельном потоке
    CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)SBMonitorLoop, NULL, 0, NULL);
}
