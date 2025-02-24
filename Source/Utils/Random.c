/*  Written in 2018 by David Blackman and Sebastiano Vigna (vigna@acm.org)

To the extent possible under law, the author has dedicated all copyright
and related and neighboring rights to this software to the public domain
worldwide. This software is distributed without any warranty.

See <http://creativecommons.org/publicdomain/zero/1.0/>. */

#include <stdint.h>

/* This is xoshiro256** 1.0, one of our all-purpose, rock-solid
   generators. It has excellent (sub-ns) speed, a state (256 bits) that is
   large enough for any parallel application, and it passes all tests we
   are aware of.

   For generating just floating-point numbers, xoshiro256+ is even faster.

   The state must be seeded so that it is not everywhere zero. If you have
   a 64-bit seed, we suggest to seed a splitmix64 generator and use its
   output to fill s. */

static inline uint64_t rotl(const uint64_t x, int k) {
	return (x << k) | (x >> (64 - k));
}

static uint64_t s[4];

uint64_t next() {
	const uint64_t result = rotl(s[1] * 5, 7) * 9;

	const uint64_t t = s[1] << 17;

	s[2] ^= s[0];
	s[3] ^= s[1];
	s[1] ^= s[2];
	s[0] ^= s[3];

	s[2] ^= t;

	s[3] = rotl(s[3], 45);

	return result;
}

/*  Written in 2015 by Sebastiano Vigna (vigna@acm.org)

To the extent possible under law, the author has dedicated all copyright
and related and neighboring rights to this software to the public domain
worldwide. This software is distributed without any warranty.

See <http://creativecommons.org/publicdomain/zero/1.0/>. */


uint64_t sm_state = 0x0123456789ABCDEF;           /* The state can be seeded with any (upto) 64 bit integer value. */

uint64_t sm_next_int() {
	sm_state += 0x9e3779b97f4a7c15;               /* increment the state variable */
	uint64_t z = sm_state;                        /* copy the state to a working variable */
	z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9;  /* xor the variable with the variable right bit shifted 30 then multiply by a constant */
	z = (z ^ (z >> 27)) * 0x94d049bb133111eb;  /* xor the variable with the variable right bit shifted 27 then multiply by a constant */
	return z ^ (z >> 31);                      /* return the variable xored with itself right bit shifted 31 */
}

/* Wrapper */

#include "Utils/Machine.h"

void srand64(uint64_t seed) {
	sm_state = seed;
	s[0] = sm_next_int();
	s[1] = sm_next_int();
	s[2] = sm_next_int();
	s[3] = sm_next_int();
}

uint64_t rand64() {
	return next();
}

// Source: https://www.pcg-random.org/posts/bounded-rands.html

uint64_t rand64_bounded(uint64_t range) {
	--range;
	uint64_t mask = UINT64_MAX >> (63 - log2_u64(range | 1));
	uint64_t x;
	do {
		x = rand64() & mask;
	} while (x > range);
	return x;
}
