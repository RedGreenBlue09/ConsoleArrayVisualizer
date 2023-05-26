
#include "Visualizer/Visualizer.h"
#include "Utils/GuardedMalloc.h"

#include <stdio.h>

#ifdef _WIN32
#include <Windows.h>
#endif

#include "Visualizer/Renderer/ColumnVirtualTerminal.h"

// Array
typedef struct {

	AV_ARRAYPROP_RENDERER vapr;

	// TODO: Horizontal scaling

} RCVT_ARRAYPROP;

static RCVT_ARRAYPROP RendererCvt_aRcvtArrayProp[AV_MAX_ARRAY_COUNT];
// TODO: Linked list to keep track of active (added) items.

typedef struct {
	short X;
	short Y;
} RCVT_BUFFER_SIZE;

#ifdef _WIN32
// To restore later
static ULONG OldInputMode = 0, OldOutputMode = 0;
#endif

static RCVT_BUFFER_SIZE RendererCvt_BufferSize;

static const int32_t RendererCvt_VtSgrTable[256] = {
	(97 | (40 << 16)),       //AvAttribute_Background
	(90 | (47 << 16)),       //AvAttribute_Normal
	(93 | (44 << 16)),       //AvAttribute_Read
	(96 | (41 << 16)),       //AvAttribute_Write
	(91 | (46 << 16)),       //AvAttribute_Pointer
	(95 | (42 << 16)),       //AvAttribute_Correct
	(96 | (41 << 16)),       //AvAttribute_Incorrect
};

static int32_t RendererCvt_AttrToVtSgr(AvAttribute Attr) {
	return RendererCvt_VtSgrTable[Attr]; // return 0 on unknown Attr.
}

// s should have at least length of 6 (positive), 7 (negative)
static void RendererCvt_i16toa(int16_t X, char* s) {

	if (X < 0) *s++ = '-';
	if (X > 0) X = -X;
	if (X <= -10000) *s++ = -(X / 10000 % 10) + '0';
	if (X <= -1000) *s++ = -(X / 1000 % 10) + '0';
	if (X <= -100) *s++ = -(X / 100 % 10) + '0';
	if (X <= -10) *s++ = -(X / 10 % 10) + '0';
	if (X <= 0) *s++ = -(X % 10) + '0';
	*s = '\0';

	return;

}

void RendererCvt_Initialize() {

#ifdef _WIN32
	// Enable virtual terminal on Windows.

	GetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), &OldInputMode);
	GetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE), &OldOutputMode);
	SetConsoleMode(
		GetStdHandle(STD_INPUT_HANDLE),
		ENABLE_PROCESSED_INPUT | ENABLE_VIRTUAL_TERMINAL_INPUT
	);
	SetConsoleMode(
		GetStdHandle(STD_OUTPUT_HANDLE),
		ENABLE_PROCESSED_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING
	);
#endif

	// Alternate buffer

	fwrite("\x1B[?1049h", 1, sizeof("\x1B[?1049h"), stdout);

	// Set cursor to top left

	fwrite("\x1B[1G\x1B[1d", 1, sizeof("\x1B[1G\x1B[1d"), stdout);

	// Clear screen

	{
		// Change color
		char sBuffer[24];
		char* pBufferCurrent = sBuffer;

		*pBufferCurrent++ = '\x1B';
		*pBufferCurrent++ = '[';

		uint32_t VtColor = RendererCvt_AttrToVtSgr(AvAttribute_Background);
		RendererCvt_i16toa(VtColor & 0xFFFF, pBufferCurrent);
		pBufferCurrent += strlen(pBufferCurrent);
		*pBufferCurrent++ = ';';
		RendererCvt_i16toa((VtColor >> 16) & 0xFFFF, pBufferCurrent);
		pBufferCurrent += strlen(pBufferCurrent);

		*pBufferCurrent++ = 'm';

		// Clear screen
		*pBufferCurrent++ = '\x1B';
		*pBufferCurrent++ = '[';
		*pBufferCurrent++ = '2';
		*pBufferCurrent++ = 'K';
		*pBufferCurrent = '\0';

		fwrite(sBuffer, sizeof(char), strlen(pBufferCurrent), stdout);
	}

	// Get buffer size

	{
		// Move to largest posible position
		fwrite("\x1B[32767G\x1B[32767d", 1, sizeof("\x1B[32767G\x1B[32767d"), stdout);

		// Request cursor position
		fwrite("\x1B[6n", 1, sizeof("\x1B[6n"), stdout);
		fflush(stdout);

		// Read the data
		fscanf_s(
			stdin,
			"\x1B[%"PRId16";%"PRId16"R",
			&RendererCvt_BufferSize.Y,
			&RendererCvt_BufferSize.X
		);

	}

	fprintf(stderr, "%i%i\r\n", RendererCvt_BufferSize.X, RendererCvt_BufferSize.Y);

	return;
}

void RendererCvt_Uninitialize() {

	// Main buffer

	fwrite("\x1B[?1049l", 1, sizeof("\x1B[?1049l"), stdout);

#ifdef _WIN32
	// Restore old console mode on Windows
	SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), OldInputMode);
	SetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE), OldOutputMode);
#endif

	return;

}

void RendererCvt_AddArray(intptr_t ArrayId, intptr_t Size) {

	RendererCvt_aRcvtArrayProp[ArrayId].vapr.Size = Size;
	RendererCvt_aRcvtArrayProp[ArrayId].vapr.aArrayState = malloc_guarded(Size * sizeof(isort_t));
	RendererCvt_aRcvtArrayProp[ArrayId].vapr.aAttribute = malloc_guarded(Size * sizeof(AvAttribute));

	RendererCvt_aRcvtArrayProp[ArrayId].vapr.bVisible = FALSE;
	RendererCvt_aRcvtArrayProp[ArrayId].vapr.ValueMin = 0;
	RendererCvt_aRcvtArrayProp[ArrayId].vapr.ValueMax = 1;

	// Initialize arrays

	for (intptr_t i = 0; i < Size; ++i)
		RendererCvt_aRcvtArrayProp[ArrayId].vapr.aArrayState[i] = 0;

	for (intptr_t i = 0; i < Size; ++i)
		RendererCvt_aRcvtArrayProp[ArrayId].vapr.aAttribute[i] = AvAttribute_Normal;

	return;
}

void RendererCvt_RemoveArray(intptr_t ArrayId) {

	free(RendererCvt_aRcvtArrayProp[ArrayId].vapr.aAttribute);
	free(RendererCvt_aRcvtArrayProp[ArrayId].vapr.aArrayState);
	return;

}

void RendererCvt_UpdateArray(intptr_t ArrayId, isort_t NewSize, isort_t* aNewArrayState, bool bVisible, isort_t ValueMin, isort_t ValueMax) {

	// Clear screen

	for (intptr_t i = 0; i < RendererCvt_aRcvtArrayProp[ArrayId].vapr.Size; ++i) {

		RendererCvt_UpdateItem(
			ArrayId,
			i,
			AV_RENDERER_UPDATEVALUE,
			0,
			0
		);

	}

	RendererCvt_aRcvtArrayProp[ArrayId].vapr.bVisible = bVisible;
	RendererCvt_aRcvtArrayProp[ArrayId].vapr.ValueMin = ValueMin;
	RendererCvt_aRcvtArrayProp[ArrayId].vapr.ValueMax = ValueMax;

	// Handle array resize

	if ((NewSize > 0) && (NewSize != RendererCvt_aRcvtArrayProp[ArrayId].vapr.Size)) {

		// Realloc arrays

		isort_t* aResizedArrayState = realloc_guarded(
			RendererCvt_aRcvtArrayProp[ArrayId].vapr.aArrayState,
			NewSize * sizeof(isort_t)
		);

		AvAttribute* aResizedAttribute = realloc_guarded(
			RendererCvt_aRcvtArrayProp[ArrayId].vapr.aAttribute,
			NewSize * sizeof(AvAttribute)
		);


		intptr_t OldSize = RendererCvt_aRcvtArrayProp[ArrayId].vapr.Size;
		intptr_t NewPartSize = NewSize - OldSize;

		// Initialize the new part

		for (intptr_t i = 0; i < NewPartSize; ++i)
			aResizedArrayState[OldSize + i] = 0;

		for (intptr_t i = 0; i < NewPartSize; ++i)
			aResizedAttribute[OldSize + i] = AvAttribute_Normal;

		RendererCvt_aRcvtArrayProp[ArrayId].vapr.aArrayState = aResizedArrayState;
		RendererCvt_aRcvtArrayProp[ArrayId].vapr.aAttribute = aResizedAttribute;

		RendererCvt_aRcvtArrayProp[ArrayId].vapr.Size = NewSize;

	}

	isort_t* aArrayState = RendererCvt_aRcvtArrayProp[ArrayId].vapr.aArrayState;
	intptr_t Size = RendererCvt_aRcvtArrayProp[ArrayId].vapr.Size;

	// Handle new array state

	if (aNewArrayState)
		for (intptr_t i = 0; i < Size; ++i)
			aArrayState[i] = aNewArrayState[i];

	// Re-render with new props

	// Same attribute
	if (aNewArrayState) {

		for (intptr_t i = 0; i < Size; ++i) {
			RendererCvt_UpdateItem(
				ArrayId,
				i,
				AV_RENDERER_UPDATEVALUE,
				aNewArrayState[i],
				0
			);
		}

	}
	else {

		for (intptr_t i = 0; i < Size; ++i) {
			RendererCvt_UpdateItem(
				ArrayId,
				i,
				AV_RENDERER_UPDATEVALUE,
				aArrayState[i],
				0
			);
		}

	}

	return;

}

void RendererCvt_UpdateItem(
	intptr_t ArrayId,
	uintptr_t iPos,
	uint32_t UpdateRequest,
	isort_t NewValue,
	AvAttribute NewAttr
) {

	// Choose the correct value & attribute

	isort_t TargetValue = RendererCvt_aRcvtArrayProp[ArrayId].vapr.aArrayState[iPos];
	if (UpdateRequest & AV_RENDERER_UPDATEVALUE)
		TargetValue = NewValue;

	AvAttribute TargetAttr = RendererCvt_aRcvtArrayProp[ArrayId].vapr.aAttribute[iPos];
	if (UpdateRequest & AV_RENDERER_UPDATEATTR)
		TargetAttr = NewAttr;

	RendererCvt_aRcvtArrayProp[ArrayId].vapr.aArrayState[iPos] = TargetValue;
	RendererCvt_aRcvtArrayProp[ArrayId].vapr.aAttribute[iPos] = TargetAttr;

	isort_t ValueMin = RendererCvt_aRcvtArrayProp[ArrayId].vapr.ValueMin;
	isort_t ValueMax = RendererCvt_aRcvtArrayProp[ArrayId].vapr.ValueMax;

	TargetValue -= ValueMin;
	ValueMax -= ValueMin; // Warning: Overflow

	if (TargetValue > ValueMax)
		TargetValue = ValueMax;

	// Scale the value to the corresponding screen height

	double dfHeight = (double)TargetValue * (double)RendererCvt_BufferSize.Y / (double)ValueMax;
	int FloorHeight = (int)dfHeight;

	// Generate VT sequence

	char* sBuffer = malloc_guarded((8 + 8 + 14 + 1 + (1)) * RendererCvt_BufferSize.Y * sizeof(char));
	char* pBufferCurrent = sBuffer;

	int32_t VtColor = RendererCvt_AttrToVtSgr(AvAttribute_Background);
	for (intptr_t i = 0; i < (intptr_t)(RendererCvt_BufferSize.Y - FloorHeight); ++i) {

		// Change horizontal pos

		*pBufferCurrent++ = '\x1B';
		*pBufferCurrent++ = '[';

		// Max length of each number is 5 because 32767
		RendererCvt_i16toa((int16_t)iPos + 1, pBufferCurrent);
		pBufferCurrent += strlen(pBufferCurrent);

		*pBufferCurrent++ = 'G';

		// Change vertical pos

		*pBufferCurrent++ = '\x1B';
		*pBufferCurrent++ = '[';

		RendererCvt_i16toa((int16_t)i + 1, pBufferCurrent);
		pBufferCurrent += strlen(pBufferCurrent);

		*pBufferCurrent++ = 'd';

		// Change text color

		*pBufferCurrent++ = '\x1B';
		*pBufferCurrent++ = '[';

		RendererCvt_i16toa(LOWORD(VtColor), pBufferCurrent);
		pBufferCurrent += strlen(pBufferCurrent);
		*pBufferCurrent++ = ';';
		RendererCvt_i16toa(HIWORD(VtColor), pBufferCurrent);
		pBufferCurrent += strlen(pBufferCurrent);

		*pBufferCurrent++ = 'm';

		// Space
		*pBufferCurrent++ = ' ';
	}

	VtColor = RendererCvt_AttrToVtSgr(TargetAttr);
	for (intptr_t i = 0; i < (intptr_t)FloorHeight; ++i) {

		// Change horizontal pos

		*pBufferCurrent++ = '\x1B';
		*pBufferCurrent++ = '[';

		RendererCvt_i16toa((int16_t)iPos + 1, pBufferCurrent);
		pBufferCurrent += strlen(pBufferCurrent);

		*pBufferCurrent++ = 'G';

		// Change vertical pos

		*pBufferCurrent++ = '\x1B';
		*pBufferCurrent++ = '[';

		RendererCvt_i16toa((int16_t)(i + RendererCvt_BufferSize.Y - FloorHeight + 1), pBufferCurrent);
		pBufferCurrent += strlen(pBufferCurrent);

		*pBufferCurrent++ = 'd';

		// Change text color

		*pBufferCurrent++ = '\x1B';
		*pBufferCurrent++ = '[';

		RendererCvt_i16toa(LOWORD(VtColor), pBufferCurrent);
		pBufferCurrent += strlen(pBufferCurrent);
		*pBufferCurrent++ = ';';
		RendererCvt_i16toa(HIWORD(VtColor), pBufferCurrent);
		pBufferCurrent += strlen(pBufferCurrent);

		*pBufferCurrent++ = 'm';

		// Space
		*pBufferCurrent++ = ' ';
	}

	// Write to StdOut

	size_t Written = fwrite(
		sBuffer,
		sizeof(char),
		pBufferCurrent - (char*)sBuffer,
		stdout
	);

	free(sBuffer);
	return;

}
