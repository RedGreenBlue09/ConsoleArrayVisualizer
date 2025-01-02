#pragma once

#include <inttypes.h>

typedef int32_t isort_t;
typedef uint32_t usort_t;

typedef int64_t long_isort_t;
typedef uint64_t long_usort_t;

typedef void* Visualizer_Handle;

typedef uint8_t Visualizer_MarkerAttribute;
#define Visualizer_MarkerAttribute_Pointer 0
#define Visualizer_MarkerAttribute_Read 1
#define Visualizer_MarkerAttribute_Write 2
#define Visualizer_MarkerAttribute_Correct 3
#define Visualizer_MarkerAttribute_Incorrect 4
#define Visualizer_MarkerAttribute_EnumCount 5

//#define VISUALIZER_DISABLE_SLEEP 1
