#pragma once

#define strlen_literal(X) (sizeof(X) / sizeof(*(X)) - 1)
#define swap(X, Y) {typeof(*(X)) Temp = *(X); *(X) = *(Y); *(Y) = Temp;}
