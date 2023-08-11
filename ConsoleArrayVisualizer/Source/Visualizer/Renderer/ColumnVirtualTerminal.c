
#include "Visualizer/Visualizer.h"
#include "Utils/GuardedMalloc.h"

#include <stdio.h>

#ifdef _WIN32
#include <Windows.h>
#endif

#include "Visualizer/Renderer/ColumnVirtualTerminal.h"

// Array
typedef struct {

	AV_ARRAYPROP_RENDERER;

	// TODO: Horizontal scaling

} RCVT_ARRAYPROP;

static tree234* RendererCvt_ptreeGlobalArrayProp; // tree of RCVT_ARRAYPROP_RENDERER

static int RendererCvt_ArrayPropIdCmp(void* pA, void* pB) {
	RCVT_ARRAYPROP* pvapA = pA;
	RCVT_ARRAYPROP* pvapB = pB;
	return (pvapA->ArrayId > pvapB->ArrayId) - (pvapA->ArrayId < pvapB->ArrayId);
}

#ifdef _WIN32
// To restore later
static ULONG OldInputMode = 0, OldOutputMode = 0;
#endif

typedef struct {
	int16_t X;
	int16_t Y;
} RCVT_COORD;

typedef struct {

	uint8_t sgrForeground : 8; // SGR
	uint8_t sgrBackground : 8; // SGR
	bool bBold         : 1; // ON / OFF
	bool bUnderline    : 1; // ON / OFF
	bool bNegative     : 1; // ON / OFF
	bool bMbChar       : 1; // YES / NO

	// MbText is used when updating text using *CellCacheChar() functions.
	// https://devblogs.microsoft.com/commandline/windows-command-line-unicode-and-utf-8-output-text-buffer/

} RCVT_VTFORMAT;

typedef struct {

	// Each "character" is equivalent to a glyph.
	// 4-bytes default
	union {
		char strCharacter[4];
		char* strMbCharacter;
	};
	RCVT_VTFORMAT vtfFormat;

} RCVT_BUFFER_CELL;

static RCVT_COORD RendererCvt_coordBufferSize; // No. of cells width & height
static RCVT_BUFFER_CELL* RendererCvt_abcBufferCellCache;

static RCVT_VTFORMAT RendererCvt_AvAttrToVtFormat(AvAttribute Attribute) {
	// TODO: Improve
	RCVT_VTFORMAT avtfTable[32] = { 0 };
	avtfTable[AvAttribute_Background] = (RCVT_VTFORMAT){ 97, 40, false, false, false, false };
	avtfTable[AvAttribute_Normal    ] = (RCVT_VTFORMAT){ 90, 47, false, false, false, false };
	avtfTable[AvAttribute_Read      ] = (RCVT_VTFORMAT){ 93, 44, false, false, false, false };
	avtfTable[AvAttribute_Write     ] = (RCVT_VTFORMAT){ 96, 41, false, false, false, false };
	avtfTable[AvAttribute_Pointer   ] = (RCVT_VTFORMAT){ 91, 46, false, false, false, false };
	avtfTable[AvAttribute_Correct   ] = (RCVT_VTFORMAT){ 95, 42, false, false, false, false };
	avtfTable[AvAttribute_Incorrect ] = (RCVT_VTFORMAT){ 96, 41, false, false, false, false };
	return ((unsigned int)Attribute < 32) ? avtfTable[Attribute] : (RCVT_VTFORMAT){ 0, 0, false, false, false, false }; // return 0 on unknown Attr.
}

static void RendererCvt_UpdateCellCacheChar(
	RCVT_BUFFER_CELL* pbcCell,
	char* strCharacter,
	intptr_t nChars
) {
	if (nChars > 3) {

		if (pbcCell->vtfFormat.bMbChar)
			pbcCell->strMbCharacter = realloc_guarded(pbcCell->strMbCharacter, (nChars + 1) * sizeof(char));
		else
			pbcCell->strMbCharacter = malloc_guarded((nChars + 1) * sizeof(char));

		memcpy(pbcCell->strMbCharacter, strCharacter, nChars * sizeof(char));
		pbcCell->strMbCharacter[nChars] = '\0';

	} else {

		if (pbcCell->vtfFormat.bMbChar)
			free(pbcCell->strMbCharacter);

		for (intptr_t i = 0; i < nChars; ++i)
			pbcCell->strCharacter[i] = strCharacter[i];
		strCharacter[nChars] = '\0';

	}
	return;
}

static char* RendererCvt_GetCellCacheChar(RCVT_BUFFER_CELL* pbcCell) {
	return (pbcCell->vtfFormat.bMbChar) ? pbcCell->strMbCharacter : pbcCell->strCharacter;
};

void RendererCvt_Initialize() {

	// Initialize RendererCvt_ptreeGlobalArrayProp

	RendererCvt_ptreeGlobalArrayProp = newtree234(RendererCvt_ArrayPropIdCmp);

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

	// Get buffer size
	{

		// Move to largest posible position
		fwrite("\x1B[32767G\x1B[32767d", 1, sizeof("\x1B[32767G\x1B[32767d"), stdout);

		// Clear stdin
		fseek(stdin, 0, SEEK_END);

		// Request cursor position
		fwrite("\x1B[6n", 1, sizeof("\x1B[6n"), stdout);
		fflush(stdout);

		// Read the data
		fscanf_s(
			stdin,
			"\x1B[%"PRId16";%"PRId16"R",
			&RendererCvt_coordBufferSize.Y,
			&RendererCvt_coordBufferSize.X
		);

	}
	if (RendererCvt_coordBufferSize.X <= 0 || RendererCvt_coordBufferSize.Y <= 0)
		abort();

	intptr_t BufferSize1D = (intptr_t)RendererCvt_coordBufferSize.X * (intptr_t)RendererCvt_coordBufferSize.Y;
	RendererCvt_abcBufferCellCache = malloc_guarded(BufferSize1D * sizeof(RCVT_BUFFER_CELL));

	// Set cursor to top left

	fwrite("\x1B[1G\x1B[1d", 1, sizeof("\x1B[1G\x1B[1d"), stdout);

	// Clear screen

	{

		// Change color

		RCVT_BUFFER_CELL bcCell = {
			.strCharacter = { ' ', '\0', '\0', '\0' },
			.vtfFormat = RendererCvt_AvAttrToVtFormat(AvAttribute_Background)
		};

		// Update cell cache

		for (intptr_t i = 0; i < BufferSize1D; ++i)
			RendererCvt_abcBufferCellCache[i] = bcCell;

		// Write to stdout

		fprintf(
			stdout,
			"\x1b[%"PRIu8";%"PRIu8";22;24;27m\x1b[2K",
			bcCell.vtfFormat.sgrForeground,
			bcCell.vtfFormat.sgrBackground
		);

	}

	return;
}

void RendererCvt_Uninitialize() {

	// Uninitialize RendererCvt_ptreeGlobalArrayProp

	for (
		RCVT_ARRAYPROP* prap = delpos234(RendererCvt_ptreeGlobalArrayProp, 0);
		prap != NULL;
		prap = delpos234(RendererCvt_ptreeGlobalArrayProp, 0)
	) {
		free(prap);
	}
	freetree234(RendererCvt_ptreeGlobalArrayProp);

	// Main buffer

	fwrite("\x1B[?1049l", 1, sizeof("\x1B[?1049l"), stdout);

	free(RendererCvt_abcBufferCellCache);

#ifdef _WIN32
	// Restore old console mode on Windows

	SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), OldInputMode);
	SetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE), OldOutputMode);
#endif

	return;

}

void RendererCvt_AddArray(intptr_t ArrayId, intptr_t Size) {

	RCVT_ARRAYPROP* prapArrayProp = malloc_guarded(sizeof(RCVT_ARRAYPROP));

	prapArrayProp->ArrayId = ArrayId;
	prapArrayProp->Size = Size;

	prapArrayProp->aArrayState = malloc_guarded(Size * sizeof(isort_t));
	for (intptr_t i = 0; i < Size; ++i)
		prapArrayProp->aArrayState[i] = 0;

	prapArrayProp->aAttribute = malloc_guarded(Size * sizeof(AvAttribute));
	for (intptr_t i = 0; i < Size; ++i)
		prapArrayProp->aAttribute[i] = AvAttribute_Normal;

	prapArrayProp->bVisible = false;
	prapArrayProp->ValueMin = 0;
	prapArrayProp->ValueMax = 1;

	// Add to tree

	add234(RendererCvt_ptreeGlobalArrayProp, prapArrayProp);

	return;
}

void RendererCvt_RemoveArray(intptr_t ArrayId) {

	RCVT_ARRAYPROP rapFind = { .ArrayId = ArrayId };
	RCVT_ARRAYPROP* prapArrayProp = find234(RendererCvt_ptreeGlobalArrayProp, &rapFind, NULL);

	free(prapArrayProp->aAttribute);
	free(prapArrayProp->aArrayState);

	// Remove from tree

	delpos234(RendererCvt_ptreeGlobalArrayProp, ArrayId);
	free(prapArrayProp);

	return;

}

void RendererCvt_UpdateArray(
	intptr_t ArrayId,
	isort_t NewSize,
	isort_t* aNewArrayState,
	bool bVisible,
	isort_t ValueMin,
	isort_t ValueMax
) {

	RCVT_ARRAYPROP rapFind = { .ArrayId = ArrayId };
	RCVT_ARRAYPROP* prapArrayProp = find234(RendererCvt_ptreeGlobalArrayProp, &rapFind, NULL);
	
	// Clear screen
	
	for (intptr_t i = 0; i < prapArrayProp->Size; ++i) {

		RendererCvt_UpdateItem(
			ArrayId,
			i,
			AV_RENDERER_UPDATEVALUE,
			0,
			0
		);

	}

	prapArrayProp->bVisible = bVisible;
	prapArrayProp->ValueMin = ValueMin;
	prapArrayProp->ValueMax = ValueMax;

	// Handle array resize

	if ((NewSize > 0) && (NewSize != prapArrayProp->Size)) {

		// Realloc arrays

		isort_t* aResizedArrayState = realloc_guarded(
			prapArrayProp->aArrayState,
			NewSize * sizeof(isort_t)
		);

		AvAttribute* aResizedAttribute = realloc_guarded(
			prapArrayProp->aAttribute,
			NewSize * sizeof(AvAttribute)
		);


		intptr_t OldSize = prapArrayProp->Size;
		intptr_t NewPartSize = NewSize - OldSize;

		// Initialize the new part

		for (intptr_t i = 0; i < NewPartSize; ++i)
			aResizedArrayState[OldSize + i] = 0;

		for (intptr_t i = 0; i < NewPartSize; ++i)
			aResizedAttribute[OldSize + i] = AvAttribute_Normal;

		prapArrayProp->aArrayState = aResizedArrayState;
		prapArrayProp->aAttribute = aResizedAttribute;

		prapArrayProp->Size = NewSize;

	}

	isort_t* aArrayState = prapArrayProp->aArrayState;
	intptr_t Size = prapArrayProp->Size;

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

	} else {

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

	RCVT_ARRAYPROP rapFind = { .ArrayId = ArrayId };
	RCVT_ARRAYPROP* prapArrayProp = find234(RendererCvt_ptreeGlobalArrayProp, &rapFind, NULL);

	// Choose the correct value & attribute

	isort_t TargetValue = prapArrayProp->aArrayState[iPos];
	if (UpdateRequest & AV_RENDERER_UPDATEVALUE)
		TargetValue = NewValue;

	AvAttribute TargetAttr = prapArrayProp->aAttribute[iPos];
	if (UpdateRequest & AV_RENDERER_UPDATEATTR)
		TargetAttr = NewAttr;

	prapArrayProp->aArrayState[iPos] = TargetValue;
	prapArrayProp->aAttribute[iPos] = TargetAttr;

	isort_t ValueMin = prapArrayProp->ValueMin;
	isort_t ValueMax = prapArrayProp->ValueMax;

	TargetValue -= ValueMin;
	ValueMax -= ValueMin; // Warning: Overflow

	if (TargetValue > ValueMax)
		TargetValue = ValueMax;

	// Scale the value to the corresponding screen height

	double dfHeight = (double)TargetValue * (double)RendererCvt_coordBufferSize.Y / (double)ValueMax;
	int16_t FloorHeight = (int16_t)dfHeight;

	// Generate VT sequence

	// Fill unused cells

	int16_t TargetConsoleCol = (int16_t)iPos;
	if (TargetConsoleCol >= RendererCvt_coordBufferSize.X)
		TargetConsoleCol = RendererCvt_coordBufferSize.X - 1;

	{
		// Update cell cache

		intptr_t i;
		for (i = 0; i < (intptr_t)(RendererCvt_coordBufferSize.Y - FloorHeight); ++i) {

			RCVT_VTFORMAT vtfFormat = RendererCvt_AvAttrToVtFormat(AvAttribute_Background);
			RCVT_BUFFER_CELL* pbcCell = &RendererCvt_abcBufferCellCache[RendererCvt_coordBufferSize.X * i + TargetConsoleCol];
			pbcCell->vtfFormat.sgrForeground = vtfFormat.sgrForeground;
			pbcCell->vtfFormat.sgrBackground = vtfFormat.sgrBackground;

		}

		for (i; i < RendererCvt_coordBufferSize.Y; ++i) {

			RCVT_VTFORMAT vtfFormat = RendererCvt_AvAttrToVtFormat(TargetAttr);
			RCVT_BUFFER_CELL* pbcCell = &RendererCvt_abcBufferCellCache[RendererCvt_coordBufferSize.X * i + TargetConsoleCol];
			pbcCell->vtfFormat.sgrForeground = vtfFormat.sgrForeground;
			pbcCell->vtfFormat.sgrBackground = vtfFormat.sgrBackground;

		}

	}

	// Count

	const char strFormat[] = "\x1b[%"PRIi16"G\x1b[%"PRIi16"d\x1b[%"PRIu8";%"PRIu8"m%s";
	intptr_t Length = 0;

	for (intptr_t i = 0; i < RendererCvt_coordBufferSize.Y; ++i) {

		RCVT_BUFFER_CELL* pbcCell = &RendererCvt_abcBufferCellCache[RendererCvt_coordBufferSize.X * i + TargetConsoleCol];

		Length += (intptr_t)snprintf(
			NULL,
			0,
			strFormat,
			(int16_t)TargetConsoleCol + 1,
			(int16_t)i + 1,
			pbcCell->vtfFormat.sgrForeground,
			pbcCell->vtfFormat.sgrBackground,
			RendererCvt_GetCellCacheChar(pbcCell)
		);

	}
	Length += 1; // '\0'

	// Write to buffer

	char* strBuffer = malloc_guarded(Length * sizeof(char));
	char* strBufferCurrent = strBuffer;
	intptr_t LengthCurrent = Length;

	for (intptr_t i = 0; i < RendererCvt_coordBufferSize.Y; ++i) {
		
		RCVT_BUFFER_CELL* pbcCell = &RendererCvt_abcBufferCellCache[RendererCvt_coordBufferSize.X * i + TargetConsoleCol];

		int Written = sprintf_s(
			strBufferCurrent,
			LengthCurrent,
			strFormat,
			(int16_t)TargetConsoleCol + 1,
			(int16_t)i + 1,
			pbcCell->vtfFormat.sgrForeground,
			pbcCell->vtfFormat.sgrBackground,
			RendererCvt_GetCellCacheChar(pbcCell)
		);
		strBufferCurrent += Written;
		LengthCurrent -= Written;

	}

	// Write to stdout

	fwrite(strBuffer, sizeof(char), Length, stdout);

	free(strBuffer);
	return;

}
