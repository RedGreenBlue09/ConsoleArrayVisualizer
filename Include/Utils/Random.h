#pragma once
#include <stdint.h>

typedef struct {
	uint64_t s[4];
} rand64_state;

void srand64(rand64_state* state, uint64_t seed);
uint64_t rand64(rand64_state* state);
uint64_t rand64_bounded(rand64_state* state, uint64_t max_value);
double randf64(rand64_state* state);