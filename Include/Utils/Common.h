#pragma once
#include <stdalign.h>

#define strlen_literal(X) (sizeof(X) / sizeof(*(X)) - 1)

#define swap(X, Y) {typeof(*(X)) Temp = *(X); *(X) = *(Y); *(Y) = Temp;}

#define min2(X, Y) (((X) < (Y)) ? (X) : (Y))
#define max2(X, Y) (((X) > (Y)) ? (X) : (Y))

#define atomic _Atomic
#define non_atomic(X) alignas(atomic X) X
