#pragma once
#include <stdint.h>

#include "Utils/Machine.h"

/* 64-bit RNG */

typedef struct {
	uint64_t s[4];
} rand64_state;

void srand64(rand64_state* state, uint64_t seed);
uint64_t rand64(rand64_state* state);
uint64_t rand64_bounded(rand64_state* state, uint64_t max_value);
double randf64(rand64_state* state);

/* 32-bit RNG */

typedef struct {
	uint32_t s[4];
} rand32_state;

void srand32(rand32_state* state, uint32_t seed);
void srand32_u64(rand32_state* state, uint64_t seed);
uint32_t rand32(rand32_state* state);
uint32_t rand32_bounded(rand32_state* state, uint32_t max_value);
float randf32(rand32_state* state);

/* Pointer RNG */

#if MACHINE_PTR64

typedef rand64_state randptr_state;

#define srandptr srand64
#define srandptr_u64 srand64
#define randptr rand64
#define randptr_bounded rand64_bounded

#elif MACHINE_PTR32

typedef rand32_state randptr_state;

#define srandptr srand32
#define srandptr_u64 srand32_u64
#define randptr rand32
#define randptr_bounded rand32_bounded

#endif
