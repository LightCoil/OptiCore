#pragma once
#include <stddef.h>
#include <stdbool.h>

typedef struct load_predictor load_predictor_t;

void load_predictor_init(load_predictor_t* pred, size_t history_size);
void load_predictor_add_sample(load_predictor_t* pred, float value);
float load_predictor_autocorr(load_predictor_t* pred, size_t lag);
float load_predictor_predict(load_predictor_t* pred);
void load_predictor_destroy(load_predictor_t* pred); 