#include "Utils/Random.h"

/*  Written in 2018 by David Blackman and Sebastiano Vigna (vigna@acm.org)

To the extent possible under law, the author has dedicated all copyright
and related and neighboring rights to this software to the public domain
worldwide. This software is distributed without any warranty.

See <http://creativecommons.org/publicdomain/zero/1.0/>. */

/* This is xoshiro256** 1.0 */

static inline uint64_t rotl(const uint64_t x, int k) {
	return (x << k) | (x >> (64 - k));
}

static uint64_t xoshiro_next(rand64_state* s) {
	const uint64_t result = rotl(s->s[1] * 5, 7) * 9;

	const uint64_t t = s->s[1] << 17;

	s->s[2] ^= s->s[0];
	s->s[3] ^= s->s[1];
	s->s[1] ^= s->s[2];
	s->s[0] ^= s->s[3];

	s->s[2] ^= t;

	s->s[3] = rotl(s->s[3], 45);

	return result;
}

/* This is splitmix64 */

static uint64_t splitmix_next(uint64_t* state) {
	*state += 0x9e3779b97f4a7c15;
	uint64_t z = *state;
	z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9;
	z = (z ^ (z >> 27)) * 0x94d049bb133111eb;
	return z ^ (z >> 31);
}

/* Wrapper */

#include "Utils/Machine.h"

void srand64(rand64_state* state, uint64_t seed) {
	state->s[0] = splitmix_next(&seed);
	state->s[1] = splitmix_next(&seed);
	state->s[2] = splitmix_next(&seed);
	state->s[3] = splitmix_next(&seed);
}

uint64_t rand64(rand64_state* state) {
	return xoshiro_next(state);
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
	return (double)(xoshiro_next(state) >> 11) * 0x1p-53;
}
