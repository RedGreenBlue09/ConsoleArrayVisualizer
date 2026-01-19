#pragma once

#include <float.h>

#include "Utils/Machine.h"

// Useful macros

#define static_arrlen(X) (sizeof(X) / sizeof(*(X)))
#define static_strlen(X) (static_arrlen(X) - 1)
#define sizeof_member(type, member) (sizeof(((type*)0)->member))

// Language extensions

#if _MSC_VER
#define ext_forceinline __forceinline
#elif __GNUC__
#define ext_forceinline __attribute__((always_inline))
#else
#define ext_forceinline inline
#endif

#if _MSC_VER
#define ext_noinline __declspec(noinline)
#elif __GNUC__
#define ext_noinline __attribute__((noinline))
#else
#define ext_noinline
#endif

#if _MSC_VER
#define ext_noreturn __declspec(noreturn)
#elif __GNUC__
#define ext_noreturn __attribute__((noreturn))
#else
#define ext_noreturn
#endif

#if _MSC_VER
static ext_noreturn ext_forceinline void ext_unreachable() { __assume(0); }
#elif __GNUC__
static ext_noreturn ext_forceinline void ext_unreachable() { __builtin_unreachable(); }
#else
static ext_noreturn ext_forceinline void ext_unreachable() {}
#endif

#define only_reachable(X) {if (!(X)) ext_unreachable();}

// Generic functions

#define swap(pX, pY) {         \
	typeof(pX) pX2 = (pX);     \
	typeof(pY) pY2 = (pY);     \
	typeof(*(pY)) Temp = *pX2; \
	*pX2 = *pY2;               \
	*pY2 = Temp;               \
}

// Min

#define min_template(T) static ext_forceinline T min_##T(T X, T Y) { return (X < Y) ? X : Y; }
#define min_template_2(T1, T2) static ext_forceinline T1 T2 min_##T1##_##T2(T1 T2 X, T1 T2 Y) { return (X < Y) ? X : Y; }
#define min_template_3(T1, T2, T3) static ext_forceinline T1 T2 T3 min_##T1##_##T2##_##T3(T1 T2 T3 X, T1 T2 T3 Y) { return (X < Y) ? X : Y; }
min_template(_Bool);
min_template(char);
min_template_2(unsigned, char);
min_template_2(signed, char)
min_template_2(unsigned, short);
min_template_2(signed, short);
min_template_2(unsigned, int);
min_template_2(signed, int);
min_template_2(unsigned, long);
min_template_2(signed, long);
min_template_3(unsigned, long, long);
min_template_3(signed, long, long);
min_template(float);
min_template(double);
min_template_2(long, double);
static inline void* min_void_ptr(void* X, void* Y) { return (X < Y) ? X : Y; }

#define ext_min(X, Y)                               \
	_Generic(                                       \
		(X),                                        \
		_Bool:              min__Bool,              \
		char:               min_char,               \
		unsigned char:      min_unsigned_char,      \
		signed char:        min_signed_char,        \
		unsigned short:     min_unsigned_short,     \
		signed short:       min_signed_short,       \
		unsigned int:       min_unsigned_int,       \
		signed int:         min_signed_int,         \
		unsigned long:      min_unsigned_long,      \
		signed long:        min_signed_long,        \
		unsigned long long: min_unsigned_long_long, \
		signed long long:   min_signed_long_long,   \
		float:              min_float,              \
		double:             min_double,             \
		long double:        min_long_double,        \
		void*:              min_void_ptr            \
	)(X, Y)

#define ext_min_macro(X, Y) (((X) < (Y)) ? (X) : (Y))

// Max

#define max_template(T) static ext_forceinline T max_##T(T X, T Y) { return (X > Y) ? X : Y; }
#define max_template_2(T1, T2) static ext_forceinline T1 T2 max_##T1##_##T2(T1 T2 X, T1 T2 Y) { return (X > Y) ? X : Y; }
#define max_template_3(T1, T2, T3) static ext_forceinline T1 T2 T3 max_##T1##_##T2##_##T3(T1 T2 T3 X, T1 T2 T3 Y) { return (X > Y) ? X : Y; }
max_template(_Bool);
max_template(char);
max_template_2(unsigned, char);
max_template_2(signed, char)
max_template_2(unsigned, short);
max_template_2(signed, short);
max_template_2(unsigned, int);
max_template_2(signed, int);
max_template_2(unsigned, long);
max_template_2(signed, long);
max_template_3(unsigned, long, long);
max_template_3(signed, long, long);
max_template(float);
max_template(double);
max_template_2(long, double);
static inline void* max_void_ptr(void* X, void* Y) { return (X > Y) ? X : Y; }

#define ext_max(X, Y)                               \
	_Generic(                                       \
		(X),                                        \
		_Bool:              max__Bool,              \
		char:               max_char,               \
		unsigned char:      max_unsigned_char,      \
		signed char:        max_signed_char,        \
		unsigned short:     max_unsigned_short,     \
		signed short:       max_signed_short,       \
		unsigned int:       max_unsigned_int,       \
		signed int:         max_signed_int,         \
		unsigned long:      max_unsigned_long,      \
		signed long:        max_signed_long,        \
		unsigned long long: max_unsigned_long_long, \
		signed long long:   max_signed_long_long,   \
		float:              max_float,              \
		double:             max_double,             \
		long double:        max_long_double,        \
		void*:              max_void_ptr            \
	)(X, Y)

#define ext_max_macro(X, Y) (((X) > (Y)) ? (X) : (Y))

// Make 32-bit code faster

#if MACHINE_PTR64

typedef double floatptr_t;
#define PRIfPTR "lf"
#define FPTR_EPSILON DBL_EPSILON

#elif MACHINE_PTR32

typedef float floatptr_t;
#define PRIfPTR "f"
#define FPTR_EPSILON FLT_EPSILON

#endif

typedef uintptr_t usize;
#define USIZE_MAX UINTPTR_MAX

typedef intptr_t isize;
#define ISIZE_MAX INTPTR_MAX
#define ISIZE_MIN INTPTR_MIN
