
#include "Utils/Random.h"

#include "Utils/IntMath.h"

/* 64-bit RNG */

/*  Written in 2018 by David Blackman and Sebastiano Vigna (vigna@acm.org)

To the extent possible under law, the author has dedicated all copyright
and related and neighboring rights to this software to the public domain
worldwide. This software is distributed without any warranty.

See <http://creativecommons.org/publicdomain/zero/1.0/>. */

/* This is xoshiro256** 1.0 */

static inline uint64_t rotl64(const uint64_t x, int k) {
	return (x << k) | (x >> (64 - k));
}

static uint64_t xoshiro256_next(rand64_state* s) {
	const uint64_t result = rotl64(s->s[1] * 5, 7) * 9;

	const uint64_t t = s->s[1] << 17;

	s->s[2] ^= s->s[0];
	s->s[3] ^= s->s[1];
	s->s[1] ^= s->s[2];
	s->s[0] ^= s->s[3];

	s->s[2] ^= t;

	s->s[3] = rotl64(s->s[3], 45);

	return result;
}

/* This is splitmix64 */

static uint64_t splitmix64_next(uint64_t* state) {
	*state += 0x9e3779b97f4a7c15;
	uint64_t z = *state;
	z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9;
	z = (z ^ (z >> 27)) * 0x94d049bb133111eb;
	return z ^ (z >> 31);
}

/* Wrapper */

#include "Utils/Machine.h"

void srand64(rand64_state* state, uint64_t seed) {
	state->s[0] = splitmix64_next(&seed);
	state->s[1] = splitmix64_next(&seed);
	state->s[2] = splitmix64_next(&seed);
	state->s[3] = splitmix64_next(&seed);
}

uint64_t rand64(rand64_state* state) {
	return xoshiro256_next(state);
}

// Source: https://www.pcg-random.org/posts/bounded-rands.html

uint64_t rand64_bounded(rand64_state* state, uint64_t max_value) {
	uint64_t mask = UINT64_MAX >> (63 - log2_u64(max_value | 1));
	uint64_t x;
	do {
		x = rand64(state) & mask;
	} while (x > max_value);
	return x;
}

double randf64(rand64_state* state) {
	return (double)(xoshiro256_next(state) >> 11) * 0x1p-53;
}

/* 32-bit RNG */

/* This is xoshiro128** 1.1 */
/* License: http://creativecommons.org/publicdomain/zero/1.0/ */

static inline uint32_t rotl32(const uint32_t x, int k) {
	return (x << k) | (x >> (32 - k));
}

static uint32_t xoshiro128_next(rand32_state* s) {
	const uint32_t result = rotl32(s->s[1] * 5, 7) * 9;

	const uint32_t t = s->s[1] << 9;

	s->s[2] ^= s->s[0];
	s->s[3] ^= s->s[1];
	s->s[1] ^= s->s[2];
	s->s[0] ^= s->s[3];

	s->s[2] ^= t;

	s->s[3] = rotl32(s->s[3], 11);

	return result;
}

/* This is splitmix32 */

static uint32_t splitmix32_next(uint32_t* state) {
	*state += 0x9e3779b9;
	uint32_t z = *state;
	z = (z ^ (z >> 16)) * 0x21f0aaad;
	z = (z ^ (z >> 15)) * 0x735a2d97;
	return z ^ (z >> 15);
}

/* Wrappers */

void srand32(rand32_state* state, uint32_t seed) {
	state->s[0] = splitmix32_next(&seed);
	state->s[1] = splitmix32_next(&seed);
	state->s[2] = splitmix32_next(&seed);
	state->s[3] = splitmix32_next(&seed);
}

void srand32_u64(rand32_state* state, uint64_t seed) {
	uint32_t seed0 = (uint32_t)seed;
	uint32_t seed1 = (uint32_t)(seed >> 32);
	state->s[0] = splitmix32_next(&seed0);
	state->s[1] = splitmix32_next(&seed0);
	state->s[2] = splitmix32_next(&seed1);
	state->s[3] = splitmix32_next(&seed1);
}

uint32_t rand32(rand32_state* state) {
	return xoshiro128_next(state);
}

// Source: https://www.pcg-random.org/posts/bounded-rands.html

uint32_t rand32_bounded(rand32_state* state, uint32_t max_value) {
	uint32_t mask = UINT32_MAX >> (31 - log2_u32(max_value | 1));
	uint32_t x;
	do {
		x = rand32(state) & mask;
	} while (x > max_value);
	return x;
}

float randf32(rand32_state* state) {
	return (float)(xoshiro128_next(state) >> 8) * 0x1p-24f;
}
