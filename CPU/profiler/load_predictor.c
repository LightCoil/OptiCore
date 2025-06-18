#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <math.h>

// LoadPredictor: автокорреляционный анализ нагрузки

typedef struct {
    float* history;
    size_t history_size;
    size_t pos;
    float last_prediction;
} load_predictor_t;

// Инициализация предиктора
void load_predictor_init(load_predictor_t* pred, size_t history_size) {
    pred->history = (float*)malloc(sizeof(float) * history_size);
    pred->history_size = history_size;
    pred->pos = 0;
    pred->last_prediction = 0.0f;
}

// Добавить новую метрику нагрузки
void load_predictor_add_sample(load_predictor_t* pred, float value) {
    pred->history[pred->pos % pred->history_size] = value;
    pred->pos++;
}

// Автокорреляционный прогноз
float load_predictor_autocorr(load_predictor_t* pred, size_t lag) {
    if (lag >= pred->history_size) return 0.0f;
    float mean = 0.0f;
    for (size_t i = 0; i < pred->history_size; ++i) mean += pred->history[i];
    mean /= pred->history_size;
    float num = 0.0f, denom = 0.0f;
    for (size_t i = 0; i < pred->history_size - lag; ++i) {
        num += (pred->history[i] - mean) * (pred->history[i + lag] - mean);
    }
    for (size_t i = 0; i < pred->history_size; ++i) {
        denom += (pred->history[i] - mean) * (pred->history[i] - mean);
    }
    return denom ? num / denom : 0.0f;
}

// Прогноз следующей нагрузки
float load_predictor_predict(load_predictor_t* pred) {
    // Используем автокорреляцию с лагом 1
    float corr = load_predictor_autocorr(pred, 1);
    float last = pred->history[(pred->pos - 1) % pred->history_size];
    pred->last_prediction = last * corr;
    return pred->last_prediction;
}

// Освобождение ресурсов
void load_predictor_destroy(load_predictor_t* pred) {
    free(pred->history);
    pred->history = NULL;
    pred->history_size = 0;
    pred->pos = 0;
    pred->last_prediction = 0.0f;
} 