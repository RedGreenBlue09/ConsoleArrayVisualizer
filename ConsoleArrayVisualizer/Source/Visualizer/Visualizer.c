
#include <stdlib.h>
#include <stdint.h>
#include "Sorts.h"

#define FALSE 0
#define TRUE 1

#define AR_MAX_ARRAY_COUNT (16)
#define AR_MAX_POINTER_COUNT (256)

enum VisualizerAttribute_tag {
	Background,
	Normal,
	Read,
	Write,
	Pointer,
	Correct,
	Incorrect
};
typedef enum VisualizerAttribute_tag VisualizerAttribute;

static const uint64_t Visualizer_TimeDefaultDelay = 100; // miliseconds
static uint8_t Visualizer_bInitialized = FALSE;

// TODO: More types (attribute, bool)
// V_ARRAY
struct V_ARRAY_tag {

	int32_t   bActive;    // Non-updatable

	intptr_t  Size;       // Non-updatable
	isort_t*  aArrayState; // Non-updatable - Only affected by write calls to Visualizer.
	uint8_t*  aAttribute;  // Non-updatable

	int32_t   bVisible;   // Updatable
	isort_t   ValueMin;   // Updatable
	isort_t   ValueMax;   // Updatable

	intptr_t  nPointer;   // Non-updatable
	intptr_t* aPointer;   // Non-updatable

	// TODO: Tree stucture to store pointers

};

typedef struct V_ARRAY_tag V_ARRAY;

// Todo: tree struct
static V_ARRAY Visualizer_aVArrayList[AR_MAX_ARRAY_COUNT];

// Low level renderer functions

void Visualizer_UpdateItem(intptr_t ArrayId, intptr_t iPos, isort_t Value, uint8_t Attr) {

	if (!Visualizer_bInitialized) return;
	if (!Visualizer_aVArrayList[ArrayId].bActive) return;
	if (iPos >= Visualizer_aVArrayList[ArrayId].Size) return;

	RendererWcc_DrawItem(ArrayId, iPos, Value, Attr);

	return;
}

// Init

void Visualizer_Initialize() {

	if (Visualizer_bInitialized) return;

	// Initialize Renderer
	RendererWcc_Initialize();

	for (intptr_t i = 0; i < AR_MAX_ARRAY_COUNT; ++i) {

		Visualizer_aVArrayList[i].bActive = FALSE;

		Visualizer_aVArrayList[i].Size = 0;
		Visualizer_aVArrayList[i].aArrayState = NULL;
		Visualizer_aVArrayList[i].aAttribute = NULL;

		Visualizer_aVArrayList[i].bVisible = FALSE;
		Visualizer_aVArrayList[i].ValueMin = 0;
		Visualizer_aVArrayList[i].ValueMax = 0;

		Visualizer_aVArrayList[i].nPointer = 0;
		Visualizer_aVArrayList[i].aPointer = NULL;

	}

	Visualizer_bInitialized = TRUE;
	return;

}

void Visualizer_Uninitialize() {

	if (!Visualizer_bInitialized) return;

	RendererWcc_Uninitialize();
	Visualizer_bInitialized = FALSE;

	return;
}

// Sleep

void Visualizer_Sleep(double fSleepMultiplier) {

	if (!Visualizer_bInitialized) return;

	sleep64((uint64_t)((double)Visualizer_TimeDefaultDelay * fSleepMultiplier));
	return;

}

// Array

void Visualizer_AddArray(intptr_t ArrayId, intptr_t Size) {

	if (!Visualizer_bInitialized) return;
	if (Visualizer_aVArrayList[ArrayId].bActive) return;

	Visualizer_aVArrayList[ArrayId].bActive = TRUE;

	Visualizer_aVArrayList[ArrayId].Size = Size;
	Visualizer_aVArrayList[ArrayId].aArrayState = malloc(Size * sizeof(isort_t));
	Visualizer_aVArrayList[ArrayId].aAttribute = malloc(Size * sizeof(uint8_t));

	Visualizer_aVArrayList[ArrayId].bVisible = FALSE;
	Visualizer_aVArrayList[ArrayId].ValueMin = 0;
	Visualizer_aVArrayList[ArrayId].ValueMax = 0;

	Visualizer_aVArrayList[ArrayId].nPointer = AR_MAX_POINTER_COUNT;
	Visualizer_aVArrayList[ArrayId].aPointer = malloc(AR_MAX_POINTER_COUNT * sizeof(intptr_t));

	if (!Visualizer_aVArrayList[ArrayId].aArrayState)
		abort();
	if (!Visualizer_aVArrayList[ArrayId].aAttribute)
		abort();
	if (!Visualizer_aVArrayList[ArrayId].aPointer)
		abort();// TODO: Whole program: force abort on error

	// Set array state items to 0
	for (intptr_t i = 0; i < Size; ++i)
		Visualizer_aVArrayList[ArrayId].aArrayState[i] = 0;

	// Set all attributes to AR_ATTR_NORMAL
	for (intptr_t i = 0; i < Size; ++i)
		Visualizer_aVArrayList[ArrayId].aAttribute[i] = AR_ATTR_NORMAL;

	// Set all pointers to -1
	for (intptr_t i = 0; i < AR_MAX_POINTER_COUNT; ++i)
		Visualizer_aVArrayList[ArrayId].aPointer[i] = (-1);

	RendererWcc_AddArray(ArrayId, &Visualizer_aVArrayList[ArrayId]);

	return;

}

void Visualizer_RemoveArray(intptr_t ArrayId) {

	if (!Visualizer_bInitialized) return;
	if (!Visualizer_aVArrayList[ArrayId].bActive) return;

	//
	Visualizer_aVArrayList[ArrayId].bActive = FALSE;

	Visualizer_aVArrayList[ArrayId].Size = 0;
	Visualizer_aVArrayList[ArrayId].aArrayState = NULL;
	free(Visualizer_aVArrayList[ArrayId].aAttribute);
	Visualizer_aVArrayList[ArrayId].aAttribute = NULL;

	Visualizer_aVArrayList[ArrayId].bVisible = FALSE;
	Visualizer_aVArrayList[ArrayId].ValueMin = 0;
	Visualizer_aVArrayList[ArrayId].ValueMax = 0;

	Visualizer_aVArrayList[ArrayId].nPointer = 0;
	free(Visualizer_aVArrayList[ArrayId].aPointer);
	Visualizer_aVArrayList[ArrayId].aPointer = NULL;

	RendererWcc_RemoveArray(ArrayId);

	return;

}

void Visualizer_UpdateArray(intptr_t ArrayId, isort_t* aNewArrayState, int32_t bVisible, isort_t ValueMin, isort_t ValueMax) {

	if (!Visualizer_bInitialized) return;
	if (!Visualizer_aVArrayList[ArrayId].bActive) return;

	Visualizer_aVArrayList[ArrayId].bVisible = bVisible;
	Visualizer_aVArrayList[ArrayId].ValueMin = ValueMin;
	Visualizer_aVArrayList[ArrayId].ValueMax = ValueMax;

	intptr_t Size = Visualizer_aVArrayList[ArrayId].Size;
	isort_t* aArrayState = Visualizer_aVArrayList[ArrayId].aArrayState;

	for (intptr_t i = 0; i < Size; ++i)
		aArrayState[i] = aNewArrayState[i];

	RendererWcc_UpdateArray(ArrayId, aNewArrayState, bVisible, ValueMin, ValueMax);

	return;
}

// Read & Write
// These functions restore original attributes before they return.

void Visualizer_UpdateRead(intptr_t ArrayId, intptr_t iPos, double fSleepMultiplier) {

	if (!Visualizer_bInitialized) return;
	if (!Visualizer_aVArrayList[ArrayId].bActive) return;
	if (iPos >= Visualizer_aVArrayList[ArrayId].Size) return;

	intptr_t Size = Visualizer_aVArrayList[ArrayId].Size;
	isort_t* aArrayState = Visualizer_aVArrayList[ArrayId].aArrayState;

	uint8_t AttrOld = Visualizer_aVArrayList[ArrayId].aAttribute[iPos];

	RendererWcc_DrawItem(ArrayId, iPos, aArrayState[iPos], AR_ATTR_READ);
	Visualizer_Sleep(fSleepMultiplier);
	RendererWcc_DrawItem(ArrayId, iPos, aArrayState[iPos], AttrOld);

	return;

}

// Update 2 items with a single sleep (used for comparisions).
void Visualizer_UpdateRead2(intptr_t ArrayId, intptr_t iPosA, intptr_t iPosB, double fSleepMultiplier) {

	if (!Visualizer_bInitialized) return;
	if (!Visualizer_aVArrayList[ArrayId].bActive) return;
	if (iPosA >= Visualizer_aVArrayList[ArrayId].Size) return;
	if (iPosB >= Visualizer_aVArrayList[ArrayId].Size) return;

	intptr_t Size = Visualizer_aVArrayList[ArrayId].Size;
	isort_t* aArrayState = Visualizer_aVArrayList[ArrayId].aArrayState;

	uint8_t AttrOldA = Visualizer_aVArrayList[ArrayId].aAttribute[iPosA];
	uint8_t AttrOldB = Visualizer_aVArrayList[ArrayId].aAttribute[iPosB];

	RendererWcc_DrawItem(ArrayId, iPosA, aArrayState[iPosA], AR_ATTR_READ);
	RendererWcc_DrawItem(ArrayId, iPosB, aArrayState[iPosB], AR_ATTR_READ);
	Visualizer_Sleep(fSleepMultiplier);
	RendererWcc_DrawItem(ArrayId, iPosA, aArrayState[iPosA], AttrOldA);
	RendererWcc_DrawItem(ArrayId, iPosB, aArrayState[iPosB], AttrOldB);

	return;

}

// For time precision, the sort will need to do the write(s) by itself.
void Visualizer_UpdateWrite(intptr_t ArrayId, intptr_t iPos, isort_t Value, double fSleepMultiplier) {

	if (!Visualizer_bInitialized) return;
	if (!Visualizer_aVArrayList[ArrayId].bActive) return;
	if (iPos >= Visualizer_aVArrayList[ArrayId].Size) return;

	isort_t* aArrayState = Visualizer_aVArrayList[ArrayId].aArrayState;
	intptr_t Size = Visualizer_aVArrayList[ArrayId].Size;

	uint8_t AttrOld = Visualizer_aVArrayList[ArrayId].aAttribute[iPos];

	RendererWcc_DrawItem(ArrayId, iPos, aArrayState[iPos], AR_ATTR_WRITE);
	Visualizer_Sleep(fSleepMultiplier);
	RendererWcc_DrawItem(ArrayId, iPos, Value, AttrOld);

	aArrayState[iPos] = Value;

	return;

}

void Visualizer_UpdateSwap(intptr_t ArrayId, intptr_t iPosA, intptr_t iPosB, isort_t ValueA, isort_t ValueB, double fSleepMultiplier) {

	if (!Visualizer_bInitialized) return;
	if (!Visualizer_aVArrayList[ArrayId].bActive) return;
	if (iPosA >= Visualizer_aVArrayList[ArrayId].Size) return;
	if (iPosB >= Visualizer_aVArrayList[ArrayId].Size) return;

	isort_t* aArrayState = Visualizer_aVArrayList[ArrayId].aArrayState;

	uint8_t AttrOldA = Visualizer_aVArrayList[ArrayId].aAttribute[iPosA];
	uint8_t AttrOldB = Visualizer_aVArrayList[ArrayId].aAttribute[iPosB];

	// Swap the values
	RendererWcc_DrawItem(ArrayId, iPosA, aArrayState[iPosA], AR_ATTR_WRITE);
	RendererWcc_DrawItem(ArrayId, iPosB, aArrayState[iPosB], AR_ATTR_WRITE);
	Visualizer_Sleep(fSleepMultiplier);
	RendererWcc_DrawItem(ArrayId, iPosA, ValueA, AttrOldA);
	RendererWcc_DrawItem(ArrayId, iPosB, ValueB, AttrOldB);

	aArrayState[iPosA] = ValueA;
	aArrayState[iPosB] = ValueB;

	return;

}

// Pointer:
// Highlight an item and keep it highlighted until the caller removes the highlight.


static uint8_t Visualizer_IsPointerOverlapped(intptr_t ArrayId, intptr_t PointerId) {

	intptr_t nPointer = Visualizer_aVArrayList[ArrayId].nPointer;
	intptr_t* aPointer = Visualizer_aVArrayList[ArrayId].aPointer;

	uint8_t isOverlapping = FALSE;
	for (intptr_t i = 0; i < nPointer; ++i) {
		if (
			(i != PointerId) &&
			(aPointer[i] == aPointer[PointerId])
			) {
			isOverlapping = TRUE;
			break;
		}
	}

	return isOverlapping;

}

void Visualizer_UpdatePointer(intptr_t ArrayId, intptr_t PointerId, intptr_t iNewPos, double fSleepMultiplier) {

	if (!Visualizer_bInitialized) return;

	intptr_t Size = Visualizer_aVArrayList[ArrayId].Size;
	isort_t* aArrayState = Visualizer_aVArrayList[ArrayId].aArrayState;
	intptr_t nPointer = Visualizer_aVArrayList[ArrayId].nPointer;
	intptr_t* aPointer = Visualizer_aVArrayList[ArrayId].aPointer;

	if (iNewPos >= Size) return;
	if (PointerId >= nPointer) return;

	if (
		(aPointer[PointerId] != (-1)) &&
		(!Visualizer_IsPointerOverlapped(ArrayId, PointerId))
		) {

		// Reset old pointer to normal.
		Visualizer_aVArrayList[ArrayId].aAttribute[aPointer[PointerId]] = AR_ATTR_NORMAL;

		RendererWcc_DrawItem(ArrayId, aPointer[PointerId], aArrayState[aPointer[PointerId]], AR_ATTR_NORMAL);

	}

	Visualizer_aVArrayList[ArrayId].aAttribute[iNewPos] = AR_ATTR_POINTER;

	RendererWcc_DrawItem(ArrayId, iNewPos, aArrayState[iNewPos], AR_ATTR_POINTER);

	Visualizer_Sleep(fSleepMultiplier);
	aPointer[PointerId] = iNewPos;

	return;

}

void Visualizer_RemovePointer(intptr_t ArrayId, intptr_t PointerId) {

	if (!Visualizer_bInitialized) return;

	intptr_t Size = Visualizer_aVArrayList[ArrayId].Size;
	isort_t* aArrayState = Visualizer_aVArrayList[ArrayId].aArrayState;
	intptr_t nPointer = Visualizer_aVArrayList[ArrayId].nPointer;
	intptr_t* aPointer = Visualizer_aVArrayList[ArrayId].aPointer;

	if (PointerId >= nPointer) return;

	if (
		(aPointer[PointerId] != (-1)) &&
		(!Visualizer_IsPointerOverlapped(ArrayId, PointerId))
		) {

		// Reset old pointer to normal.
		Visualizer_aVArrayList[ArrayId].aAttribute[aPointer[PointerId]] = AR_ATTR_NORMAL;

		RendererWcc_DrawItem(ArrayId, aPointer[PointerId], aArrayState[aPointer[PointerId]], AR_ATTR_NORMAL);

	}

	aPointer[PointerId] = (intptr_t)(-1);

	return;

}
