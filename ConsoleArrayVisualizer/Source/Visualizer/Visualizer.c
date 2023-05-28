
#include <string.h>
#include "Sorts.h"

#include "Visualizer/Visualizer.h"

#include "Visualizer/Renderer/ColumnWindowsConsole.h"
#include "Visualizer/Renderer/ColumnVirtualTerminal.h"

#include "Utils/GuardedMalloc.h"
#include "Utils/Time.h"

#ifndef VISUALIZER_DISABLED

AV_RENDERER_ENTRY Visualizer_AvRendererEntry;

// TODO: More argument checks

static const uint64_t Visualizer_TimeDefaultDelay = 10000; // microseconds
static bool Visualizer_bInitialized = false;

// TODO: Maybe tree struct for this.
static AV_ARRAYPROP Visualizer_aAvArrayProp[AV_MAX_ARRAY_COUNT];

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

	memset(Visualizer_aAvArrayProp, 0, sizeof(Visualizer_aAvArrayProp));

	// Only for now
	
	Visualizer_AvRendererEntry.Initialize   = RendererCvt_Initialize,
	Visualizer_AvRendererEntry.Uninitialize = RendererCvt_Uninitialize,

	Visualizer_AvRendererEntry.AddArray    = RendererCvt_AddArray,
	Visualizer_AvRendererEntry.RemoveArray = RendererCvt_RemoveArray,
	Visualizer_AvRendererEntry.UpdateArray = RendererCvt_UpdateArray,

	Visualizer_AvRendererEntry.UpdateItem  = RendererCvt_UpdateItem,

	// Call renderer

	Visualizer_AvRendererEntry.Initialize();

	Visualizer_bInitialized = true;

	return;

}

void Visualizer_Uninitialize() {

	if (!Visualizer_bInitialized) return;

	Visualizer_AvRendererEntry.Uninitialize();
	Visualizer_bInitialized = false;

	return;
}

#ifndef VISUALIZER_DISABLE_SLEEP

void Visualizer_Sleep(double fSleepMultiplier) {

	if (!Visualizer_bInitialized) return;

	sleep64((uint64_t)((double)Visualizer_TimeDefaultDelay * fSleepMultiplier));
	return;

}

#endif

// Array

static int Visualizer_PointerCmp(void* pA, void* pB);

static int Visualizer_UniqueMarkerCmp(void* pA, void* pB);
static int Visualizer_UniqueMarkerIdCmp(void* pA, void* pB);
static int Visualizer_UniqueMarkerPriorityCmp(void* pA, void* pB);

void Visualizer_AddArray(intptr_t ArrayId, intptr_t Size) {

	// AddArray & RemoveArray routines for UniqueMarker & Pointer

	if (!Visualizer_bInitialized) return;
	if (Visualizer_aAvArrayProp[ArrayId].bActive) return;
	if (Size < 1) return;

	Visualizer_aAvArrayProp[ArrayId].bActive = true;
	Visualizer_aAvArrayProp[ArrayId].Size = Size;

	Visualizer_aAvArrayProp[ArrayId].ptreeUniqueMarker = newtree234(Visualizer_UniqueMarkerCmp);

	Visualizer_aAvArrayProp[ArrayId].ptreeUniqueMarkerEmptyId = newtree234(Visualizer_UniqueMarkerIdCmp);
	intptr_t* pFirstMarkerId = malloc_guarded(sizeof(intptr_t));
	*pFirstMarkerId = AV_UNIQUEMARKER_FIRST_ID;
	add234(Visualizer_aAvArrayProp[ArrayId].ptreeUniqueMarkerEmptyId, pFirstMarkerId);

	Visualizer_aAvArrayProp[ArrayId].aptreeUniqueMarkerMap = malloc_guarded(Size * sizeof(tree234*));

	Visualizer_aAvArrayProp[ArrayId].ptreePointer = newtree234(Visualizer_PointerCmp);

	// Initialize arrays

	for (intptr_t i = 0; i < Size; ++i)
		Visualizer_aAvArrayProp[ArrayId].aptreeUniqueMarkerMap[i] = newtree234(Visualizer_UniqueMarkerPriorityCmp);

	// Call renderer

	Visualizer_AvRendererEntry.AddArray(ArrayId, Size);

	return;

}

void Visualizer_RemoveArray(intptr_t ArrayId) {

	if (!Visualizer_bInitialized) return;
	if (!Visualizer_aAvArrayProp[ArrayId].bActive) return;

	intptr_t Size = Visualizer_aAvArrayProp[ArrayId].Size;

	freetree234(Visualizer_aAvArrayProp[ArrayId].ptreeUniqueMarker);
	freetree234(Visualizer_aAvArrayProp[ArrayId].ptreeUniqueMarkerEmptyId);

	for (intptr_t i = 0; i < Size; ++i) {

		freetree234(Visualizer_aAvArrayProp[ArrayId].aptreeUniqueMarkerMap[i]);
	}
	free(Visualizer_aAvArrayProp[ArrayId].aptreeUniqueMarkerMap);

	freetree234(Visualizer_aAvArrayProp[ArrayId].ptreePointer);

	// TODO: Free all items in trees
	// Currently, they all leak

	memset(&Visualizer_aAvArrayProp[ArrayId], 0, sizeof(Visualizer_aAvArrayProp[ArrayId]));

	Visualizer_AvRendererEntry.RemoveArray(ArrayId);

	return;

}

void Visualizer_UpdateArray(intptr_t ArrayId, isort_t NewSize, isort_t* aNewArrayState, int32_t bVisible, isort_t ValueMin, isort_t ValueMax) {

	if (!Visualizer_bInitialized) return;
	if (!Visualizer_aAvArrayProp[ArrayId].bActive) return;
	if (ValueMax < ValueMin) return;

	// Handle array resize

	if ((NewSize > 0) && (NewSize != Visualizer_aAvArrayProp[ArrayId].Size)) {

		// Resize the marker map

		tree234** aptreeResizedUmMap = realloc_guarded(Visualizer_aAvArrayProp[ArrayId].aptreeUniqueMarkerMap, NewSize);

		intptr_t OldSize = Visualizer_aAvArrayProp[ArrayId].Size;
		intptr_t NewPartSize = NewSize - OldSize;

		// Initialize the new part

		for (intptr_t i = 0; i < NewPartSize; ++i)
			aptreeResizedUmMap[OldSize + i] = newtree234(Visualizer_UniqueMarkerPriorityCmp);

		Visualizer_aAvArrayProp[ArrayId].aptreeUniqueMarkerMap = aptreeResizedUmMap;


		Visualizer_aAvArrayProp[ArrayId].Size = NewSize;

	}

	// Call renderer

	Visualizer_AvRendererEntry.UpdateArray(ArrayId, NewSize, aNewArrayState, bVisible, ValueMin, ValueMax);

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
	
	// TODO: Compare marker id if similar attributes
}

intptr_t Visualizer_NewUniqueMarker(
	intptr_t ArrayId,
	intptr_t iPos,
	uint8_t bUpdateValue,
	isort_t NewValue,
	AvAttribute Attribute
) {

	tree234* ptreeUniqueMarker = Visualizer_aAvArrayProp[ArrayId].ptreeUniqueMarker;
	tree234* ptreeUniqueMarkerEmptyId = Visualizer_aAvArrayProp[ArrayId].ptreeUniqueMarkerEmptyId;
	tree234** aptreeUniqueMarkerMap = Visualizer_aAvArrayProp[ArrayId].aptreeUniqueMarkerMap;

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

	Visualizer_AvRendererEntry.UpdateItem(
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

	tree234* ptreeUniqueMarker = Visualizer_aAvArrayProp[ArrayId].ptreeUniqueMarker;
	tree234* ptreeUniqueMarkerEmptyId = Visualizer_aAvArrayProp[ArrayId].ptreeUniqueMarkerEmptyId;
	tree234** aptreeUniqueMarkerMap = Visualizer_aAvArrayProp[ArrayId].aptreeUniqueMarkerMap;

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

	Visualizer_AvRendererEntry.UpdateItem(
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
	if (!Visualizer_aAvArrayProp[ArrayId].bActive) return;
	if (iPos >= Visualizer_aAvArrayProp[ArrayId].Size || iPos < 0) return;

	intptr_t MarkerId = Visualizer_NewUniqueMarker(
		ArrayId,
		iPos,
		false,
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
	if (!Visualizer_aAvArrayProp[ArrayId].bActive) return;
	if (iPosA >= Visualizer_aAvArrayProp[ArrayId].Size || iPosA < 0) return;
	if (iPosB >= Visualizer_aAvArrayProp[ArrayId].Size || iPosB < 0) return;

	intptr_t Size = Visualizer_aAvArrayProp[ArrayId].Size;

	intptr_t MarkerIdA = Visualizer_NewUniqueMarker(
		ArrayId,
		iPosA,
		false,
		0,
		AvAttribute_Read
	);
	intptr_t MarkerIdB = Visualizer_NewUniqueMarker(
		ArrayId,
		iPosB,
		false,
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
	if (!Visualizer_aAvArrayProp[ArrayId].bActive) return;
	if (iPos >= Visualizer_aAvArrayProp[ArrayId].Size || iPos < 0) return;

	intptr_t MarkerId = Visualizer_NewUniqueMarker(
		ArrayId,
		iPos,
		true,
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
	if (!Visualizer_aAvArrayProp[ArrayId].bActive) return;
	if (iPosA >= Visualizer_aAvArrayProp[ArrayId].Size || iPosA < 0) return;
	if (iPosB >= Visualizer_aAvArrayProp[ArrayId].Size || iPosB < 0) return;


	intptr_t MarkerIdA = Visualizer_NewUniqueMarker(
		ArrayId,
		iPosA,
		true,
		NewValueA,
		AvAttribute_Write
	);
	intptr_t MarkerIdB = Visualizer_NewUniqueMarker(
		ArrayId,
		iPosB,
		true,
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
	if (iNewPos >= Visualizer_aAvArrayProp[ArrayId].Size || iNewPos < 0) return;

	intptr_t Size = Visualizer_aAvArrayProp[ArrayId].Size;

	tree234* ptreePointer = Visualizer_aAvArrayProp[ArrayId].ptreePointer;

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
		false,
		0,
		AvAttribute_Pointer
	);

	return;

}

void Visualizer_RemovePointer(intptr_t ArrayId, intptr_t PointerId) {

	if (!Visualizer_bInitialized) return;

	intptr_t Size = Visualizer_aAvArrayProp[ArrayId].Size;
	//isort_t* aArrayState = Visualizer_aAvArrayProp[ArrayId].aArrayState;

	tree234* ptreePointer = Visualizer_aAvArrayProp[ArrayId].ptreePointer;

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
