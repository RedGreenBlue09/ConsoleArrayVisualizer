
#include <stdlib.h>
#include "Sorts.h"
#include "Visualizer.h"

#include "Utils.h"
#include "GuardedMalloc.h"

#ifndef VISUALIZER_DISABLED

// TODO: More argument checks

static const uint64_t Visualizer_TimeDefaultDelay = 10000; // microseconds
static uint8_t Visualizer_bInitialized = FALSE;

// TODO: Maybe tree struct for this.
static AV_ARRAYPROP Visualizer_aArrayProp[AV_MAX_ARRAY_COUNT];

#define AV_UNIQUEMARKER_INVALID_ID INTPTR_MIN // reserved id
#define AV_UNIQUEMARKER_FIRST_ID (INTPTR_MIN + 1)

typedef struct {
	intptr_t UniqueId;
	intptr_t iPos;
	AvAttribute Attribute;
} AV_UNIQUEMARKER;

// Init

void Visualizer_Initialize() {

	if (Visualizer_bInitialized) return;

	RendererWcc_Initialize();

	memset(Visualizer_aArrayProp, 0, sizeof(Visualizer_aArrayProp));

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

int Visualizer_PointerCmp(void* pA, void* pB);

int Visualizer_UniqueMarkerCmp(void* pA, void* pB);
int Visualizer_UniqueMarkerIdCmp(void* pA, void* pB);
int Visualizer_UniqueMarkerPriorityCmp(void* pA, void* pB);

void Visualizer_AddArray(intptr_t ArrayId, intptr_t Size) {

	if (!Visualizer_bInitialized) return;
	if (Visualizer_aArrayProp[ArrayId].bActive) return;
	if (Size < 1) return;

	Visualizer_aArrayProp[ArrayId].bActive = TRUE;
	Visualizer_aArrayProp[ArrayId].Size = Size;

	Visualizer_aArrayProp[ArrayId].ptreeUniqueMarker = newtree234(Visualizer_UniqueMarkerCmp);

	Visualizer_aArrayProp[ArrayId].ptreeUniqueMarkerEmptyId = newtree234(Visualizer_UniqueMarkerIdCmp);
	intptr_t* pFirstMarkerId = malloc_guarded(sizeof(intptr_t));
	*pFirstMarkerId = AV_UNIQUEMARKER_FIRST_ID;
	add234(Visualizer_aArrayProp[ArrayId].ptreeUniqueMarkerEmptyId, pFirstMarkerId);

	Visualizer_aArrayProp[ArrayId].aptreeUniqueMarkerMap = malloc_guarded(Size * sizeof(tree234*));

	Visualizer_aArrayProp[ArrayId].ptreePointer = newtree234(Visualizer_PointerCmp);

	// Initialize arrays

	for (intptr_t i = 0; i < Size; ++i)
		Visualizer_aArrayProp[ArrayId].aptreeUniqueMarkerMap[i] = newtree234(Visualizer_UniqueMarkerPriorityCmp);

	// Call visualizer

	RendererWcc_AddArray(ArrayId, Size);

	return;

}

void Visualizer_RemoveArray(intptr_t ArrayId) {

	if (!Visualizer_bInitialized) return;
	if (!Visualizer_aArrayProp[ArrayId].bActive) return;

	intptr_t Size = Visualizer_aArrayProp[ArrayId].Size;

	freetree234(Visualizer_aArrayProp[ArrayId].ptreeUniqueMarker);
	freetree234(Visualizer_aArrayProp[ArrayId].ptreeUniqueMarkerEmptyId);

	for (intptr_t i = 0; i < Size; ++i)
		freetree234(Visualizer_aArrayProp[ArrayId].aptreeUniqueMarkerMap[i]);
	free(Visualizer_aArrayProp[ArrayId].aptreeUniqueMarkerMap);

	freetree234(Visualizer_aArrayProp[ArrayId].ptreePointer);

	// TODO: Free all items in trees
	// Currently, they all leak

	memset(&Visualizer_aArrayProp[ArrayId], 0, sizeof(Visualizer_aArrayProp[ArrayId]));

	RendererWcc_RemoveArray(ArrayId);

	return;

}

void Visualizer_UpdateArray(intptr_t ArrayId, isort_t NewSize, isort_t* aNewArrayState, int32_t bVisible, isort_t ValueMin, isort_t ValueMax) {

	if (!Visualizer_bInitialized) return;
	if (!Visualizer_aArrayProp[ArrayId].bActive) return;
	if (ValueMax < ValueMin) return;

	// Handle array resize

	if ((NewSize > 0) && (NewSize != Visualizer_aArrayProp[ArrayId].Size)) {

		// Resize the marker map

		tree234** aptreeResizedUmMap = realloc_guarded(Visualizer_aArrayProp[ArrayId].aptreeUniqueMarkerMap, NewSize);

		intptr_t OldSize = Visualizer_aArrayProp[ArrayId].Size;
		intptr_t NewPartSize = NewSize - OldSize;

		// Initialize the new part

		for (intptr_t i = 0; i < NewPartSize; ++i)
			aptreeResizedUmMap[OldSize + i] = newtree234(Visualizer_UniqueMarkerPriorityCmp);

		Visualizer_aArrayProp[ArrayId].aptreeUniqueMarkerMap = aptreeResizedUmMap;


		Visualizer_aArrayProp[ArrayId].Size = NewSize;

	}

	// Call renderer

	RendererWcc_UpdateArray(ArrayId, NewSize, aNewArrayState, bVisible, ValueMin, ValueMax);

	return;
}

// Marker

static int Visualizer_UniqueMarkerCmp(void* pA, void* pB) {
	// Compare by id
	AV_UNIQUEMARKER* pvumA = pA;
	AV_UNIQUEMARKER* pvumB = pB;
	return (pvumA->UniqueId > pvumB->UniqueId) - (pvumA->UniqueId < pvumB->UniqueId);
}

static int Visualizer_UniqueMarkerIdCmp(void* pA, void* pB) {
	return (*(intptr_t*)pA > *(intptr_t*)pB) - (*(intptr_t*)pA < *(intptr_t*)pB);
}

static const int Visualizer_UniqueMarkerAttrPriority[] = {
	0, //AvAttribute_Background
	1, //AvAttribute_Normal
	3, //AvAttribute_Read
	4, //AvAttribute_Write
	2, //AvAttribute_Pointer
	5, //AvAttribute_Correct
	6, //AvAttribute_Incorrect
};

static int Visualizer_UniqueMarkerPriorityCmp(void* pA, void* pB) {
	// Compare by attribute
	AV_UNIQUEMARKER* pvumA = pA;
	AV_UNIQUEMARKER* pvumB = pB;
	int PrioA = Visualizer_UniqueMarkerAttrPriority[pvumA->Attribute];
	int PrioB = Visualizer_UniqueMarkerAttrPriority[pvumB->Attribute];
	return (PrioA > PrioB) - (PrioA < PrioB);
}

intptr_t Visualizer_NewUniqueMarker(
	intptr_t ArrayId,
	intptr_t iPos,
	uint8_t bUpdateValue,
	isort_t NewValue,
	AvAttribute Attribute
) {

	tree234* ptreeUniqueMarker = Visualizer_aArrayProp[ArrayId].ptreeUniqueMarker;
	tree234* ptreeUniqueMarkerEmptyId = Visualizer_aArrayProp[ArrayId].ptreeUniqueMarkerEmptyId;
	tree234** aptreeUniqueMarkerMap = Visualizer_aArrayProp[ArrayId].aptreeUniqueMarkerMap;

	// Acquire new ID (unique)

	if (count234(ptreeUniqueMarkerEmptyId) == UINTPTR_MAX) // Out of ids
		return AV_UNIQUEMARKER_INVALID_ID; // INTPTR_MIN is reserved for error handling
	intptr_t* pEmptyId = index234(ptreeUniqueMarkerEmptyId, 0);
	intptr_t MarkerId = *pEmptyId;

	// Update the empty chunk

	// Check if the next id is already used
	AV_UNIQUEMARKER vumSearch = (AV_UNIQUEMARKER){ MarkerId + 1, 0, 0 };
	if (find234(ptreeUniqueMarker, &vumSearch, NULL) != NULL) {
		// If yes, this empty chunk is filled -> no longer empty
		del234(ptreeUniqueMarkerEmptyId, pEmptyId);
		free(pEmptyId);
	} else {
		// This empty chunk is not filled
		(*pEmptyId) += 1;
	}

	// Add the new marker to the marker list

	AV_UNIQUEMARKER* pvumNewUniqueMarker = malloc_guarded(sizeof(AV_UNIQUEMARKER));
	pvumNewUniqueMarker->UniqueId = MarkerId;
	pvumNewUniqueMarker->iPos = iPos;
	pvumNewUniqueMarker->Attribute = Attribute;
	add234(ptreeUniqueMarker, pvumNewUniqueMarker);

	// Add the new marker to the map
	
	add234(aptreeUniqueMarkerMap[iPos], pvumNewUniqueMarker);
	// pvumNewUniqueMarker is shared between
	// ptreeUniqueMarker and aptreeUniqueMarkerMap[iPos]

	// Update visually (call renderer)
	
	// Get the highest priority marker
	AV_UNIQUEMARKER* pvumHighestPriority = findrel234(aptreeUniqueMarkerMap[iPos], NULL, NULL, REL234_LT);

	uint32_t UpdateRequest = AV_RENDERER_UPDATEATTR;
	if (bUpdateValue)
		UpdateRequest |= AV_RENDERER_UPDATEVALUE;

	RendererWcc_UpdateItem(
		ArrayId,
		iPos,
		UpdateRequest,
		NewValue,
		pvumHighestPriority->Attribute
	);

	return MarkerId;

}

intptr_t Visualizer_RemoveUniqueMarker(
	intptr_t ArrayId,
	intptr_t MarkerId
) {

	tree234* ptreeUniqueMarker = Visualizer_aArrayProp[ArrayId].ptreeUniqueMarker;
	tree234* ptreeUniqueMarkerEmptyId = Visualizer_aArrayProp[ArrayId].ptreeUniqueMarkerEmptyId;
	tree234** aptreeUniqueMarkerMap = Visualizer_aArrayProp[ArrayId].aptreeUniqueMarkerMap;

	// Delete from the marker list & acquire iPos 

	AV_UNIQUEMARKER vumSearch = (AV_UNIQUEMARKER){ MarkerId, 0, 0 };
	AV_UNIQUEMARKER* pvumUniqueMarker = del234(ptreeUniqueMarker, &vumSearch);

	intptr_t iPos = pvumUniqueMarker->iPos;

	// Delete from the map & free memory

	del234(aptreeUniqueMarkerMap[iPos], pvumUniqueMarker);
	free(pvumUniqueMarker);

	// Free the id & update empty chunks

	intptr_t PrevId = MarkerId - 1;
	intptr_t NextId = MarkerId + 1;

	// Check if the next id is already used
	vumSearch = (AV_UNIQUEMARKER){ NextId, 0, 0 };
	AV_UNIQUEMARKER* pVumNext = find234(ptreeUniqueMarker, &vumSearch, NULL);
	if (pVumNext == NULL) {

		// The next id is not used -> an empty chunk is there
		// Decrement that empty chunk to this marker id
		intptr_t* pVumNext = find234(ptreeUniqueMarkerEmptyId, &NextId, NULL);
		(*pVumNext) -= 1;


		if (MarkerId == AV_UNIQUEMARKER_FIRST_ID) {

			// It's already the first id so no previous id
			// TODO: Add AV_UNIQUEMARKER_FIRST_ID - 1
			//       to ptreeUniqueMarker to eliminate the need
			//       to handle such case. It is a hack though.

		} else {

			// Check if the previous id is already used
			vumSearch = (AV_UNIQUEMARKER){ PrevId, 0, 0 };
			AV_UNIQUEMARKER* pVumPrev = find234(ptreeUniqueMarker, &vumSearch, NULL);
			if (pVumPrev == NULL) {

				// Empty chunks (before & after) is merged
				del234(ptreeUniqueMarkerEmptyId, pVumNext);
				free(pVumNext);

			}

		}

	} else {

		if (MarkerId == AV_UNIQUEMARKER_FIRST_ID) {

			// No need to check the previous one.
			// A new empty chunk have just formed.
			intptr_t* pNewHoleId = malloc_guarded(sizeof(intptr_t));
			*pNewHoleId = MarkerId;
			add234(ptreeUniqueMarkerEmptyId, pNewHoleId);

		} else {

			// Check if the previous id is already used
			vumSearch = (AV_UNIQUEMARKER){ PrevId, 0, 0 };
			AV_UNIQUEMARKER* pVumPrev = find234(ptreeUniqueMarker, &vumSearch, NULL);
			if (pVumPrev == NULL) {
				// The previous id is not used
				// Already in an existing empty chunk so do nothing
			} else {

				// A new empty chunk have just formed.
				intptr_t* pNewHoleId = malloc_guarded(sizeof(intptr_t));
				*pNewHoleId = MarkerId;
				add234(ptreeUniqueMarkerEmptyId, pNewHoleId);

			}

		}

	}

	// Update visually (call renderer)

	// Get the highest priority marker
	AV_UNIQUEMARKER* pvumHighestPriority = findrel234(aptreeUniqueMarkerMap[iPos], NULL, NULL, REL234_LT);

	// If 0 marker are in the map slot, reset to normal
	AvAttribute TargetAttribute = AvAttribute_Normal;
	if (pvumHighestPriority)
		TargetAttribute = pvumHighestPriority->Attribute;

	RendererWcc_UpdateItem(
		ArrayId,
		iPos,
		AV_RENDERER_UPDATEATTR,
		0,
		TargetAttribute
	);

	return 0;
}

// Read & Write
// These functions restore original attributes before they return.

void Visualizer_UpdateRead(intptr_t ArrayId, intptr_t iPos, double fSleepMultiplier) {

	if (!Visualizer_bInitialized) return;
	if (!Visualizer_aArrayProp[ArrayId].bActive) return;
	if (iPos >= Visualizer_aArrayProp[ArrayId].Size || iPos < 0) return;

	intptr_t MarkerId = Visualizer_NewUniqueMarker(
		ArrayId,
		iPos,
		FALSE,
		0,
		AvAttribute_Read
	);

	//
	Visualizer_Sleep(fSleepMultiplier);

	Visualizer_RemoveUniqueMarker(ArrayId, MarkerId);

	return;

}

// Update 2 items (used for comparisions).
void Visualizer_UpdateRead2(intptr_t ArrayId, intptr_t iPosA, intptr_t iPosB, double fSleepMultiplier) {

	if (!Visualizer_bInitialized) return;
	if (!Visualizer_aArrayProp[ArrayId].bActive) return;
	if (iPosA >= Visualizer_aArrayProp[ArrayId].Size || iPosA < 0) return;
	if (iPosB >= Visualizer_aArrayProp[ArrayId].Size || iPosB < 0) return;

	intptr_t Size = Visualizer_aArrayProp[ArrayId].Size;

	intptr_t MarkerIdA = Visualizer_NewUniqueMarker(
		ArrayId,
		iPosA,
		FALSE,
		0,
		AvAttribute_Read
	);
	intptr_t MarkerIdB = Visualizer_NewUniqueMarker(
		ArrayId,
		iPosB,
		FALSE,
		0,
		AvAttribute_Read
	);

	//
	Visualizer_Sleep(fSleepMultiplier);

	Visualizer_RemoveUniqueMarker(ArrayId, MarkerIdA);
	Visualizer_RemoveUniqueMarker(ArrayId, MarkerIdB);

	return;

}

// For time precision, the sort will need to do the write(s) by itself.
void Visualizer_UpdateWrite(intptr_t ArrayId, intptr_t iPos, isort_t NewValue, double fSleepMultiplier) {

	if (!Visualizer_bInitialized) return;
	if (!Visualizer_aArrayProp[ArrayId].bActive) return;
	if (iPos >= Visualizer_aArrayProp[ArrayId].Size || iPos < 0) return;

	intptr_t MarkerId = Visualizer_NewUniqueMarker(
		ArrayId,
		iPos,
		TRUE,
		NewValue,
		AvAttribute_Write
	);

	//
	Visualizer_Sleep(fSleepMultiplier);

	Visualizer_RemoveUniqueMarker(ArrayId, MarkerId);

	return;

}

void Visualizer_UpdateWrite2(
	intptr_t ArrayId,
	intptr_t iPosA,
	intptr_t iPosB,
	isort_t NewValueA,
	isort_t NewValueB,
	double fSleepMultiplier
) {

	if (!Visualizer_bInitialized) return;
	if (!Visualizer_aArrayProp[ArrayId].bActive) return;
	if (iPosA >= Visualizer_aArrayProp[ArrayId].Size || iPosA < 0) return;
	if (iPosB >= Visualizer_aArrayProp[ArrayId].Size || iPosB < 0) return;


	intptr_t MarkerIdA = Visualizer_NewUniqueMarker(
		ArrayId,
		iPosA,
		TRUE,
		NewValueA,
		AvAttribute_Write
	);
	intptr_t MarkerIdB = Visualizer_NewUniqueMarker(
		ArrayId,
		iPosB,
		TRUE,
		NewValueB,
		AvAttribute_Write
	);

	//
	Visualizer_Sleep(fSleepMultiplier);

	Visualizer_RemoveUniqueMarker(ArrayId, MarkerIdA);
	Visualizer_RemoveUniqueMarker(ArrayId, MarkerIdB);

	return;

}

// Pointer:
// Highlight an item and keep it highlighted until the caller removes the highlight.

typedef struct {
	intptr_t PointerId;
	intptr_t MarkerId; // The array index that it's pointing to.
} AV_POINTER;

static int Visualizer_PointerCmp(void* pA, void* pB) {

	intptr_t PointerIdA = ((AV_POINTER*)pA)->PointerId;
	intptr_t PointerIdB = ((AV_POINTER*)pB)->PointerId;

	return (PointerIdA > PointerIdB) - (PointerIdA < PointerIdB);

};

void Visualizer_UpdatePointer(intptr_t ArrayId, intptr_t PointerId, intptr_t iNewPos, double fSleepMultiplier) {

	if (!Visualizer_bInitialized) return;
	if (iNewPos >= Visualizer_aArrayProp[ArrayId].Size || iNewPos < 0) return;

	intptr_t Size = Visualizer_aArrayProp[ArrayId].Size;

	tree234* ptreePointer = Visualizer_aArrayProp[ArrayId].ptreePointer;

	// Check if pointer is already exist.

	AV_POINTER vpCompare = { PointerId, 0 };
	AV_POINTER* pvpPointer = find234(ptreePointer, &vpCompare, NULL);

	// NULL means doesn't exist
	if (pvpPointer) {
		// Remove old marker
		Visualizer_RemoveUniqueMarker(ArrayId, pvpPointer->MarkerId);
	} else {
		// Make new pointer
		pvpPointer = malloc_guarded(sizeof(AV_POINTER));
		*pvpPointer = (AV_POINTER){ PointerId, 0 };
		add234(ptreePointer, pvpPointer);
	}

	// Create new marker
	pvpPointer->MarkerId = Visualizer_NewUniqueMarker(
		ArrayId,
		iNewPos,
		FALSE,
		0,
		AvAttribute_Pointer
	);

	return;

}

void Visualizer_RemovePointer(intptr_t ArrayId, intptr_t PointerId) {

	if (!Visualizer_bInitialized) return;

	intptr_t Size = Visualizer_aArrayProp[ArrayId].Size;
	//isort_t* aArrayState = Visualizer_aArrayProp[ArrayId].aArrayState;

	tree234* ptreePointer = Visualizer_aArrayProp[ArrayId].ptreePointer;

	// Check if pointer is already exist.

	AV_POINTER vpCompare = { PointerId, 0 };
	AV_POINTER* pvpPointer = find234(ptreePointer, &vpCompare, NULL);

	if (pvpPointer) {

		Visualizer_RemoveUniqueMarker(ArrayId, pvpPointer->MarkerId);

		del234(ptreePointer, pvpPointer);
		free(pvpPointer);

	}

	return;

}

#endif
