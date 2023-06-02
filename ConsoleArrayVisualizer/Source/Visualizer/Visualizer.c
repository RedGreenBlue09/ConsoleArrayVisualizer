
#include <string.h>
#include "Sorts.h"

#include "Visualizer/Visualizer.h"

#include "Visualizer/Renderer/ColumnWindowsConsole.h"
#include "Visualizer/Renderer/ColumnVirtualTerminal.h"

#include "Utils/GuardedMalloc.h"
#include "Utils/Time.h"

#ifndef VISUALIZER_DISABLED

AV_RENDERER_ENTRY Visualizer_vreRendererEntry;

// TODO: More argument checks

static const uint64_t Visualizer_TimeDefaultDelay = 10000; // microseconds
static bool Visualizer_bInitialized = false;

// TODO: Maybe tree struct for this.
static tree234* Visualizer_ptreeGlobalArrayProp; // tree of AV_ARRAYPROP

#define AV_UNIQUEMARKER_INVALID_ID INTPTR_MIN // reserved id
#define AV_UNIQUEMARKER_FIRST_ID (INTPTR_MIN + 1)

typedef struct {
	intptr_t UniqueId;
	intptr_t iPos;
	AvAttribute Attribute;
} AV_UNIQUEMARKER;

typedef struct {
	intptr_t PointerId;
	intptr_t MarkerId; // The array index that it's pointing to.
} AV_POINTER;

// Init

static int Visualizer_ArrayPropIdCmp(void* pA, void* pB) {
	AV_ARRAYPROP* pvapA = pA;
	AV_ARRAYPROP* pvapB = pB;
	return (pvapA->ArrayId > pvapB->ArrayId) - (pvapA->ArrayId < pvapB->ArrayId);
}

void Visualizer_Initialize() {

	if (Visualizer_bInitialized) return;

	// Initialize Visualizer_ptreeGlobalArrayProp

	Visualizer_ptreeGlobalArrayProp = newtree234(Visualizer_ArrayPropIdCmp);

	// Only for now
	
	Visualizer_vreRendererEntry.Initialize   = RendererCwc_Initialize;
	Visualizer_vreRendererEntry.Uninitialize = RendererCwc_Uninitialize;

	Visualizer_vreRendererEntry.AddArray     = RendererCwc_AddArray;
	Visualizer_vreRendererEntry.RemoveArray  = RendererCwc_RemoveArray;
	Visualizer_vreRendererEntry.UpdateArray  = RendererCwc_UpdateArray;

	Visualizer_vreRendererEntry.UpdateItem   = RendererCwc_UpdateItem;

	// Call renderer

	Visualizer_vreRendererEntry.Initialize();

	Visualizer_bInitialized = true;

	return;

}

void Visualizer_Uninitialize() {

	if (!Visualizer_bInitialized) return;

	Visualizer_bInitialized = false;

	// Uninitialize Visualizer_ptreeGlobalArrayProp

	for (
		AV_ARRAYPROP* pvap = delpos234(Visualizer_ptreeGlobalArrayProp, 0);
		pvap != NULL;
		pvap = delpos234(Visualizer_ptreeGlobalArrayProp, 0)
	) {
		free(pvap);
	}
	freetree234(Visualizer_ptreeGlobalArrayProp);

	// Call renderer
	Visualizer_vreRendererEntry.Uninitialize();

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

	if (!Visualizer_bInitialized) return;
	if (Size < 1) return;

	// Check if this array is already existed.

	AV_ARRAYPROP vapFind = { .ArrayId = ArrayId };
	AV_ARRAYPROP* pvapCheck = find234(Visualizer_ptreeGlobalArrayProp, &vapFind, NULL);
	if (pvapCheck) return;

	AV_ARRAYPROP* pvapArrayProp = malloc_guarded(sizeof(AV_ARRAYPROP));
	
	//

	pvapArrayProp->ArrayId = ArrayId;
	pvapArrayProp->Size = Size;

	// Initialize ptreeUniqueMarker

	pvapArrayProp->ptreeUniqueMarker = newtree234(Visualizer_UniqueMarkerCmp);

	// Initialize ptreeUniqueMarkerEmptyId

	pvapArrayProp->ptreeUniqueMarkerEmptyId = newtree234(Visualizer_UniqueMarkerIdCmp);

	// Add the first empty chunk
	intptr_t* pFirstMarkerId = malloc_guarded(sizeof(intptr_t));
	*pFirstMarkerId = AV_UNIQUEMARKER_FIRST_ID;
	add234(pvapArrayProp->ptreeUniqueMarkerEmptyId, pFirstMarkerId);

	// Initialize aptreeUniqueMarkerMap

	pvapArrayProp->aptreeUniqueMarkerMap = malloc_guarded(Size * sizeof(tree234*));
	for (intptr_t i = 0; i < Size; ++i)
		pvapArrayProp->aptreeUniqueMarkerMap[i] = newtree234(Visualizer_UniqueMarkerPriorityCmp);

	// Initialize ptreePointer

	pvapArrayProp->ptreePointer = newtree234(Visualizer_PointerCmp);

	// Add to tree

	add234(Visualizer_ptreeGlobalArrayProp, pvapArrayProp);

	// Call renderer

	Visualizer_vreRendererEntry.AddArray(ArrayId, Size);

	return;

}

void Visualizer_RemoveArray(intptr_t ArrayId) {

	if (!Visualizer_bInitialized) return;

	AV_ARRAYPROP vapFind = { .ArrayId = ArrayId };
	AV_ARRAYPROP* pvapArrayProp = find234(Visualizer_ptreeGlobalArrayProp, &vapFind, NULL);
	if (!pvapArrayProp) return;

	intptr_t Size = pvapArrayProp->Size;

	// Uninitialize ptreeUniqueMarker

	for (
		void* p = delpos234(pvapArrayProp->ptreeUniqueMarker, 0);
		p != NULL;
		p = delpos234(pvapArrayProp->ptreeUniqueMarker, 0)
	) {
		free(p);
	}
	freetree234(pvapArrayProp->ptreeUniqueMarker);

	// Uninitialize ptreeUniqueMarkerEmptyId

	for (
		void* p = delpos234(pvapArrayProp->ptreeUniqueMarkerEmptyId, 0);
		p != NULL;
		p = delpos234(pvapArrayProp->ptreeUniqueMarkerEmptyId, 0)
	) {
		free(p);
	}
	freetree234(pvapArrayProp->ptreeUniqueMarkerEmptyId);

	// Uninitialize aptreeUniqueMarkerMap

	for (intptr_t i = 0; i < Size; ++i) {
		for (
			void* p = delpos234(pvapArrayProp->aptreeUniqueMarkerMap[i], 0);
			p != NULL;
			p = delpos234(pvapArrayProp->aptreeUniqueMarkerMap[i], 0)
		) {
			free(p);
		}
		freetree234(pvapArrayProp->aptreeUniqueMarkerMap[i]);
	}
	free(pvapArrayProp->aptreeUniqueMarkerMap);

	// Uninitialize ptreePointer

	for (
		void* p = delpos234(pvapArrayProp->ptreePointer, 0);
		p != NULL;
		p = delpos234(pvapArrayProp->ptreePointer, 0)
	) {
		free(p);
	}
	freetree234(pvapArrayProp->ptreePointer);

	// Remove from tree

	del234(Visualizer_ptreeGlobalArrayProp, pvapArrayProp);
	free(pvapArrayProp);

	// Call renderer

	Visualizer_vreRendererEntry.RemoveArray(ArrayId);

	return;

}

void Visualizer_UpdateArray(intptr_t ArrayId, isort_t NewSize, isort_t* aNewArrayState, int32_t bVisible, isort_t ValueMin, isort_t ValueMax) {

	if (!Visualizer_bInitialized) return;
	if (ValueMax <= ValueMin) return;

	AV_ARRAYPROP vapFind = { .ArrayId = ArrayId };
	AV_ARRAYPROP* pvapArrayProp = find234(Visualizer_ptreeGlobalArrayProp, &vapFind, NULL);
	if (!pvapArrayProp) return;

	// Handle array resize

	if ((NewSize > 0) && (NewSize != pvapArrayProp->Size)) {

		// Resize the marker map

		tree234** aptreeResizedUmMap = realloc_guarded(pvapArrayProp->aptreeUniqueMarkerMap, NewSize);

		intptr_t OldSize = pvapArrayProp->Size;
		intptr_t NewPartSize = NewSize - OldSize;

		// Initialize the new part

		for (intptr_t i = 0; i < NewPartSize; ++i)
			aptreeResizedUmMap[OldSize + i] = newtree234(Visualizer_UniqueMarkerPriorityCmp);

		pvapArrayProp->aptreeUniqueMarkerMap = aptreeResizedUmMap;


		pvapArrayProp->Size = NewSize;

	}

	// Call renderer

	Visualizer_vreRendererEntry.UpdateArray(ArrayId, NewSize, aNewArrayState, bVisible, ValueMin, ValueMax);

	return;
}

// Marker

static int Visualizer_UniqueMarkerCmp(void* pA, void* pB) {
	// Compare by id
	AV_UNIQUEMARKER* pumA = pA;
	AV_UNIQUEMARKER* pumB = pB;
	return (pumA->UniqueId > pumB->UniqueId) - (pumA->UniqueId < pumB->UniqueId);
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
	AV_UNIQUEMARKER* pumA = pA;
	AV_UNIQUEMARKER* pumB = pB;
	int PrioA = Visualizer_UniqueMarkerAttrPriority[pumA->Attribute];
	int PrioB = Visualizer_UniqueMarkerAttrPriority[pumB->Attribute];
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

	AV_ARRAYPROP vapFind = { .ArrayId = ArrayId };
	AV_ARRAYPROP* pvapArrayProp = find234(Visualizer_ptreeGlobalArrayProp, &vapFind, NULL);

	tree234* ptreeUniqueMarker = pvapArrayProp->ptreeUniqueMarker;
	tree234* ptreeUniqueMarkerEmptyId = pvapArrayProp->ptreeUniqueMarkerEmptyId;
	tree234** aptreeUniqueMarkerMap = pvapArrayProp->aptreeUniqueMarkerMap;

	// Acquire new ID (unique)

	if (count234(ptreeUniqueMarkerEmptyId) == UINTPTR_MAX) // Out of ids
		return AV_UNIQUEMARKER_INVALID_ID; // INTPTR_MIN is reserved for error handling
	intptr_t* pEmptyId = index234(ptreeUniqueMarkerEmptyId, 0);
	intptr_t MarkerId = *pEmptyId;

	// Update the empty chunk

	// Check if the next id is already used
	AV_UNIQUEMARKER umSearch = (AV_UNIQUEMARKER){ MarkerId + 1, 0, 0 };
	if (find234(ptreeUniqueMarker, &umSearch, NULL) != NULL) {
		// If yes, this empty chunk is filled -> no longer empty
		del234(ptreeUniqueMarkerEmptyId, pEmptyId);
		free(pEmptyId);
	} else {
		// This empty chunk is not filled
		(*pEmptyId) += 1;
	}

	// Add the new marker to the marker list

	AV_UNIQUEMARKER* pumNewUniqueMarker = malloc_guarded(sizeof(AV_UNIQUEMARKER));
	pumNewUniqueMarker->UniqueId = MarkerId;
	pumNewUniqueMarker->iPos = iPos;
	pumNewUniqueMarker->Attribute = Attribute;
	add234(ptreeUniqueMarker, pumNewUniqueMarker);

	// Add the new marker to the map
	
	add234(aptreeUniqueMarkerMap[iPos], pumNewUniqueMarker);
	// pumNewUniqueMarker is shared between
	// ptreeUniqueMarker and aptreeUniqueMarkerMap[iPos]

	// Update visually (call renderer)
	
	// Get the highest priority marker
	AV_UNIQUEMARKER* pumHighestPriority = findrel234(aptreeUniqueMarkerMap[iPos], NULL, NULL, REL234_LT);

	uint32_t UpdateRequest = AV_RENDERER_UPDATEATTR;
	if (bUpdateValue)
		UpdateRequest |= AV_RENDERER_UPDATEVALUE;

	Visualizer_vreRendererEntry.UpdateItem(
		ArrayId,
		iPos,
		UpdateRequest,
		NewValue,
		pumHighestPriority->Attribute
	);

	return MarkerId;

}

intptr_t Visualizer_RemoveUniqueMarker(
	intptr_t ArrayId,
	intptr_t MarkerId
) {

	AV_ARRAYPROP vapFind = { .ArrayId = ArrayId };
	AV_ARRAYPROP* pvapArrayProp = find234(Visualizer_ptreeGlobalArrayProp, &vapFind, NULL);

	tree234* ptreeUniqueMarker = pvapArrayProp->ptreeUniqueMarker;
	tree234* ptreeUniqueMarkerEmptyId = pvapArrayProp->ptreeUniqueMarkerEmptyId;
	tree234** aptreeUniqueMarkerMap = pvapArrayProp->aptreeUniqueMarkerMap;

	// Delete from the marker list & acquire iPos 

	AV_UNIQUEMARKER umSearch = (AV_UNIQUEMARKER){ MarkerId, 0, 0 };
	AV_UNIQUEMARKER* pumUniqueMarker = del234(ptreeUniqueMarker, &umSearch);

	intptr_t iPos = pumUniqueMarker->iPos;

	// Delete from the map & free memory

	del234(aptreeUniqueMarkerMap[iPos], pumUniqueMarker);
	free(pumUniqueMarker);

	// Free the id & update empty chunks

	intptr_t PrevId = MarkerId - 1;
	intptr_t NextId = MarkerId + 1;

	// Check if the next id is already used
	umSearch = (AV_UNIQUEMARKER){ NextId, 0, 0 };
	AV_UNIQUEMARKER* pumNext = find234(ptreeUniqueMarker, &umSearch, NULL);
	if (pumNext == NULL) {

		// The next id is not used -> an empty chunk is there
		// Decrement that empty chunk to this marker id
		intptr_t* pumNext = find234(ptreeUniqueMarkerEmptyId, &NextId, NULL);
		(*pumNext) -= 1;


		if (MarkerId == AV_UNIQUEMARKER_FIRST_ID) {

			// It's already the first id so no previous id

		} else {

			// Check if the previous id is already used
			umSearch = (AV_UNIQUEMARKER){ PrevId, 0, 0 };
			AV_UNIQUEMARKER* pumPrev = find234(ptreeUniqueMarker, &umSearch, NULL);
			if (pumPrev == NULL) {

				// Empty chunks (before & after) is merged
				del234(ptreeUniqueMarkerEmptyId, pumNext);
				free(pumNext);

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
			umSearch = (AV_UNIQUEMARKER){ PrevId, 0, 0 };
			AV_UNIQUEMARKER* pumPrev = find234(ptreeUniqueMarker, &umSearch, NULL);
			if (pumPrev == NULL) {

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
	AV_UNIQUEMARKER* pumHighestPriority = findrel234(aptreeUniqueMarkerMap[iPos], NULL, NULL, REL234_LT);

	// If 0 marker are in the map slot, reset to normal
	AvAttribute TargetAttribute = AvAttribute_Normal;
	if (pumHighestPriority)
		TargetAttribute = pumHighestPriority->Attribute;

	Visualizer_vreRendererEntry.UpdateItem(
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

	AV_ARRAYPROP vapFind = { .ArrayId = ArrayId };
	AV_ARRAYPROP* pvapArrayProp = find234(Visualizer_ptreeGlobalArrayProp, &vapFind, NULL);
	if (!pvapArrayProp) return;
	if (iPos >= pvapArrayProp->Size || iPos < 0) return;

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

	AV_ARRAYPROP vapFind = { .ArrayId = ArrayId };
	AV_ARRAYPROP* pvapArrayProp = find234(Visualizer_ptreeGlobalArrayProp, &vapFind, NULL);
	if (!pvapArrayProp) return;
	if (iPosA >= pvapArrayProp->Size || iPosA < 0) return;
	if (iPosB >= pvapArrayProp->Size || iPosB < 0) return;

	intptr_t Size = pvapArrayProp->Size;

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

	AV_ARRAYPROP vapFind = { .ArrayId = ArrayId };
	AV_ARRAYPROP* pvapArrayProp = find234(Visualizer_ptreeGlobalArrayProp, &vapFind, NULL);
	if (!pvapArrayProp) return;
	if (iPos >= pvapArrayProp->Size || iPos < 0) return;

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

	AV_ARRAYPROP vapFind = { .ArrayId = ArrayId };
	AV_ARRAYPROP* pvapArrayProp = find234(Visualizer_ptreeGlobalArrayProp, &vapFind, NULL);
	if (!pvapArrayProp) return;
	if (iPosA >= pvapArrayProp->Size || iPosA < 0) return;
	if (iPosB >= pvapArrayProp->Size || iPosB < 0) return;


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

static int Visualizer_PointerCmp(void* pA, void* pB) {

	intptr_t PointerIdA = ((AV_POINTER*)pA)->PointerId;
	intptr_t PointerIdB = ((AV_POINTER*)pB)->PointerId;

	return (PointerIdA > PointerIdB) - (PointerIdA < PointerIdB);

};

void Visualizer_UpdatePointer(intptr_t ArrayId, intptr_t PointerId, intptr_t iNewPos, double fSleepMultiplier) {

	if (!Visualizer_bInitialized) return;

	AV_ARRAYPROP vapFind = { .ArrayId = ArrayId };
	AV_ARRAYPROP* pvapArrayProp = find234(Visualizer_ptreeGlobalArrayProp, &vapFind, NULL);
	if (iNewPos >= pvapArrayProp->Size || iNewPos < 0) return;

	intptr_t Size = pvapArrayProp->Size;

	tree234* ptreePointer = pvapArrayProp->ptreePointer;

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

	AV_ARRAYPROP vapFind = { .ArrayId = ArrayId };
	AV_ARRAYPROP* pvapArrayProp = find234(Visualizer_ptreeGlobalArrayProp, &vapFind, NULL);

	intptr_t Size = pvapArrayProp->Size;
	//isort_t* aArrayState = pvapArrayProp->aArrayState;

	tree234* ptreePointer = pvapArrayProp->ptreePointer;

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
