// src/hud_overlay.cpp
#include "hud_overlay.h"
#include "system_utils.h"

// Инициализация HUD-оверлея
void InitHUDOverlay() {
    // Инициализируем графические ресурсы для HUD
    LoadHUDTextures();
    // Загружаем шрифты для отображения текста
    InitializeFontRenderer();
    // Настраиваем расположение элементов HUD (например, верхний левый угол)
    SetupHUDLayout();
    // Устанавливаем начальные значения метрик
    ResetHUDMetrics();
}

// Отрисовка HUD (температура, FPS, нагрузка)
void DrawHUD() {
    // Пустая реализация, так как теперь используется UpdateStatisticsHUD из statistics_manager
    // Можно оставить для обратной совместимости или как заглушку
}

// Отрисовка текста HUD с переданными данными
void DrawHUDText(const char* text) {
    // Проверяем, нужно ли обновить HUD
    if (text) {
        // Отрисовываем HUD на экране с переданным текстом
        RenderHUDText(text);
    }
}

// Обновление HUD только при значительных изменениях данных
void UpdateHUDIfChanged(const char* text) {
    static char lastText[256] = {0};
    // Проверяем, изменился ли текст HUD с последнего обновления
    if (text && strcmp(lastText, text) != 0) {
        // Отрисовываем HUD только если данные изменились
        RenderHUDText(text);
        // Сохраняем текущий текст как последний отображенный
        strncpy(lastText, text, sizeof(lastText) - 1);
        lastText[sizeof(lastText) - 1] = '\0';
    }
}

// Отключение HUD-оверлея
void ShutdownHUD() {
    // Освобождаем графические ресурсы HUD
    FreeHUDTextures();
    // Очищаем шрифтовый рендерер
    ShutdownFontRenderer();
    // Сбрасываем все внутренние состояния HUD
    ResetHUDState();
}
