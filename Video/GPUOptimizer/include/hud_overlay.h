// include/hud_overlay.h
#ifndef HUD_OVERLAY_H
#define HUD_OVERLAY_H

#ifdef __cplusplus
extern "C" {
#endif

// Инициализация HUD-оверлея
void InitHUDOverlay();

// Отрисовка HUD (температура, FPS, нагрузка)
void DrawHUD();

// Отрисовка текста HUD с переданными данными
void DrawHUDText(const char* text);

// Обновление HUD только при значительных изменениях данных
void UpdateHUDIfChanged(const char* text);

// Деинициализация HUD
void ShutdownHUD();

#ifdef __cplusplus
}
#endif

#endif // HUD_OVERLAY_H
