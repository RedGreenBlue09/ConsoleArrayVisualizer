#pragma once

#define static_arrlen(X) (sizeof(X) / sizeof(*(X)))
#define static_strlen(X) (static_arrlen(X) - 1)
#define member_size(type, member) (sizeof(((type*)0)->member))

#define swap(X, Y) {typeof(*(X)) Temp = *(X); *(X) = *(Y); *(Y) = Temp;}

#define min2(X, Y) (((X) < (Y)) ? (X) : (Y))
#define max2(X, Y) (((X) > (Y)) ? (X) : (Y))

// Language extensions

#if _MSC_VER
#define ext_forceinline __forceinline
#elif __GNUC__
#define ext_forceinline __attribute__((always_inline))
#else
#define ext_forceinline
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
