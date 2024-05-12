
#include "Visualizer/Visualizer.h"
#include "Utils/GuardedMalloc.h"

#include <stdio.h>

#ifdef _WIN32
#include <Windows.h>
#endif

#include "Visualizer/Renderer/ColumnVirtualTerminal.h"

// Array
typedef struct {

	pool_index                  ArrayIndex;
	intptr_t                    Size;
	isort_t*                    aState;
	Visualizer_MarkerAttribute* aAttribute;

	isort_t      ValueMin;
	isort_t      ValueMax;

	// TODO: Horizontal scaling

} RendererCvt_ArrayProp;

static intptr_t RendererCvt_nArrayProp = 0;
static RendererCvt_ArrayProp* RendererCvt_aArrayProp = NULL;

#ifdef _WIN32
// To restore later
static ULONG OldInputMode = 0, OldOutputMode = 0;
#endif

typedef struct {
	int16_t X;
	int16_t Y;
} RendererCvt_Coord;

typedef struct {

	uint8_t SgrForeground : 8; // SGR
	uint8_t SgrBackground : 8; // SGR
	bool bBold         : 1; // ON / OFF
	bool bUnderline    : 1; // ON / OFF
	bool bNegative     : 1; // ON / OFF
	bool bMbChar       : 1; // YES / NO

	// MbText is used when updating text using *CellCacheChar() functions.
	// https://devblogs.microsoft.com/commandline/windows-command-line-unicode-and-utf-8-output-text-buffer/

} RendererCvt_VtFormat;

typedef struct {

	// Each "character" is equivalent to a glyph.
	// 4-bytes default
	union {
		char aGlyphData[sizeof(void*)];
		char* aMbGlyphData;
	};
	RendererCvt_VtFormat Format;

} RendererCvt_BufferCell;

static RendererCvt_Coord RendererCvt_CoordBufferSize; // No. of cells width & height
static RendererCvt_BufferCell* RendererCvt_aBufferCellCache2D;

static RendererCvt_VtFormat RendererCvt_AvAttrToVtFormat(Visualizer_MarkerAttribute Attribute) {
	// TODO: Improve
	RendererCvt_VtFormat FormatTable[Visualizer_MarkerAttribute_EnumCount] = { 0 };
	FormatTable[Visualizer_MarkerAttribute_Background] = (RendererCvt_VtFormat){ 97, 40, false, false, false, false };
	FormatTable[Visualizer_MarkerAttribute_Normal    ] = (RendererCvt_VtFormat){ 90, 47, false, false, false, false };
	FormatTable[Visualizer_MarkerAttribute_Read      ] = (RendererCvt_VtFormat){ 93, 44, false, false, false, false };
	FormatTable[Visualizer_MarkerAttribute_Write     ] = (RendererCvt_VtFormat){ 96, 41, false, false, false, false };
	FormatTable[Visualizer_MarkerAttribute_Pointer   ] = (RendererCvt_VtFormat){ 91, 46, false, false, false, false };
	FormatTable[Visualizer_MarkerAttribute_Correct   ] = (RendererCvt_VtFormat){ 95, 42, false, false, false, false };
	FormatTable[Visualizer_MarkerAttribute_Incorrect ] = (RendererCvt_VtFormat){ 96, 41, false, false, false, false };
	return ((unsigned int)Attribute < Visualizer_MarkerAttribute_EnumCount) ?
		FormatTable[Attribute] :
		(RendererCvt_VtFormat){ 0, 0, false, false, false, false };
}

static void RendererCvt_UpdateCellCacheChar(
	RendererCvt_BufferCell* pbcCell,
	char* aGlyphData,
	intptr_t GlyphDataSize
) {
	if (GlyphDataSize > 3) {

		if (pbcCell->Format.bMbChar)
			pbcCell->aMbGlyphData = realloc_guarded(pbcCell->aMbGlyphData, (GlyphDataSize + 1) * sizeof(char));
		else
			pbcCell->aMbGlyphData = malloc_guarded((GlyphDataSize + 1) * sizeof(char));

		memcpy(pbcCell->aMbGlyphData, aGlyphData, GlyphDataSize * sizeof(char));
		pbcCell->aMbGlyphData[GlyphDataSize] = '\0';

	} else {

		if (pbcCell->Format.bMbChar)
			free(pbcCell->aMbGlyphData);

		for (intptr_t i = 0; i < GlyphDataSize; ++i)
			pbcCell->aGlyphData[i] = aGlyphData[i];
		aGlyphData[GlyphDataSize] = '\0';

	}
	return;
}

static char* RendererCvt_GetCellCacheChar(RendererCvt_BufferCell* pbcCell) {
	return (pbcCell->Format.bMbChar) ? pbcCell->aMbGlyphData : pbcCell->aGlyphData;
};

static void RendererCvt_ClearScreen() {
	const RendererCvt_VtFormat BackgroundFormat = RendererCvt_AvAttrToVtFormat(Visualizer_MarkerAttribute_Background);
	fprintf(
		stdout,
		"\x1b[%"PRIu8";%"PRIu8";22;24;27m\x1b[2J",
		BackgroundFormat.SgrForeground,
		BackgroundFormat.SgrBackground
	);
	return;
}

void RendererCvt_Initialize(intptr_t nMaxArray) {

	// Initialize RendererCvt_aArrayProp

	RendererCvt_nArrayProp = nMaxArray;
	RendererCvt_aArrayProp = malloc_guarded(nMaxArray * sizeof(*RendererCvt_aArrayProp));

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
			&RendererCvt_CoordBufferSize.Y,
			&RendererCvt_CoordBufferSize.X
		);

	}
	if (RendererCvt_CoordBufferSize.X <= 0 || RendererCvt_CoordBufferSize.Y <= 0)
		abort();

	intptr_t BufferSize1D = (intptr_t)RendererCvt_CoordBufferSize.X * (intptr_t)RendererCvt_CoordBufferSize.Y;
	RendererCvt_aBufferCellCache2D = malloc_guarded(BufferSize1D * sizeof(RendererCvt_BufferCell));

	// Set cursor to top left

	fwrite("\x1B[1G\x1B[1d", 1, sizeof("\x1B[1G\x1B[1d"), stdout);

	// Initialize cell cache

	RendererCvt_BufferCell bcCell = {
		.aGlyphData = { ' ', '\0', '\0', '\0' },
		.Format = RendererCvt_AvAttrToVtFormat(Visualizer_MarkerAttribute_Background)
	};

	for (intptr_t i = 0; i < BufferSize1D; ++i)
		RendererCvt_aBufferCellCache2D[i] = bcCell;

	//

	RendererCvt_ClearScreen();

	return;
}

void RendererCvt_Uninitialize() {

	// Uninitialize RendererCvt_aArrayProp

	free(RendererCvt_aArrayProp);

	// Main buffer

	fwrite("\x1B[?1049l", 1, sizeof("\x1B[?1049l"), stdout);

	free(RendererCvt_aBufferCellCache2D);

#ifdef _WIN32
	// Restore old console mode on Windows

	SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), OldInputMode);
	SetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE), OldOutputMode);
#endif

	return;

}

void RendererCvt_AddArray(
	pool_index ArrayIndex,
	intptr_t Size,
	isort_t* aArrayState,
	isort_t ValueMin,
	isort_t ValueMax
) {

	RendererCvt_ArrayProp* pArrayProp = RendererCvt_aArrayProp + (uintptr_t)ArrayIndex;

	pArrayProp->ArrayIndex = ArrayIndex;
	pArrayProp->Size = Size;

	pArrayProp->aState = malloc_guarded(Size * sizeof(isort_t));
	if (aArrayState)
		for (intptr_t i = 0; i < Size; ++i)
			pArrayProp->aState[i] = aArrayState[i];
	else
		for (intptr_t i = 0; i < Size; ++i)
			pArrayProp->aState[i] = 0;

	pArrayProp->aAttribute = malloc_guarded(Size * sizeof(Visualizer_MarkerAttribute));
	for (intptr_t i = 0; i < Size; ++i)
		pArrayProp->aAttribute[i] = Visualizer_MarkerAttribute_Normal;

	pArrayProp->ValueMin = ValueMin;
	pArrayProp->ValueMax = ValueMax;

	return;
}

void RendererCvt_RemoveArray(pool_index ArrayIndex) {

	RendererCvt_ArrayProp* pArrayProp = RendererCvt_aArrayProp + (uintptr_t)ArrayIndex;

	free(pArrayProp->aAttribute);
	free(pArrayProp->aState);

	return;

}

void RendererCvt_UpdateArray(
	pool_index ArrayIndex,
	intptr_t NewSize,
	isort_t ValueMin,
	isort_t ValueMax
) {

	RendererCvt_ArrayProp* pArrayProp = RendererCvt_aArrayProp + (uintptr_t)ArrayIndex;

	RendererCvt_ClearScreen();

	pArrayProp->ValueMin = ValueMin;
	pArrayProp->ValueMax = ValueMax;

	// Handle array resize

	if ((NewSize > 0) && (NewSize != pArrayProp->Size)) {

		// Realloc arrays

		isort_t* aResizedArrayState = realloc_guarded(
			pArrayProp->aState,
			NewSize * sizeof(isort_t)
		);
		Visualizer_MarkerAttribute* aResizedAttribute = realloc_guarded(
			pArrayProp->aAttribute,
			NewSize * sizeof(Visualizer_MarkerAttribute)
		);

		// Initialize the new part

		intptr_t OldSize = pArrayProp->Size;

		for (intptr_t i = OldSize; i < NewSize; ++i)
			aResizedArrayState[i] = 0;

		for (intptr_t i = OldSize; i < NewSize; ++i)
			aResizedAttribute[i] = Visualizer_MarkerAttribute_Normal;

		pArrayProp->aState = aResizedArrayState;
		pArrayProp->aAttribute = aResizedAttribute;
		pArrayProp->Size = NewSize;

	}

	// Re-render with new props

	intptr_t Size = pArrayProp->Size;
	for (intptr_t i = 0; i < Size; ++i) {
		RendererCvt_UpdateItem(
			ArrayIndex,
			i,
			AV_RENDERER_NOUPDATE,
			0,
			0
		);
	}

	return;

}

void RendererCvt_UpdateItem(
	pool_index ArrayIndex,
	intptr_t iPosition,
	uint32_t UpdateRequest,
	isort_t NewValue,
	Visualizer_MarkerAttribute NewAttr
) {

	RendererCvt_ArrayProp* pArrayProp = RendererCvt_aArrayProp + (uintptr_t)ArrayIndex;

	// Choose the correct value & attribute

	isort_t TargetValue;
	if (UpdateRequest & AV_RENDERER_UPDATEVALUE)
		TargetValue = NewValue;
	else
		TargetValue = pArrayProp->aState[iPosition];

	Visualizer_MarkerAttribute TargetAttr;
	if (UpdateRequest & AV_RENDERER_UPDATEATTR)
		TargetAttr = NewAttr;
	else
		TargetAttr = pArrayProp->aAttribute[iPosition];

	pArrayProp->aState[iPosition] = TargetValue;
	pArrayProp->aAttribute[iPosition] = TargetAttr;

	isort_t ValueMin = pArrayProp->ValueMin;
	isort_t ValueMax = pArrayProp->ValueMax;

	TargetValue -= ValueMin;
	ValueMax -= ValueMin; // Warning: Overflow

	if (TargetValue > ValueMax)
		TargetValue = ValueMax;

	// Scale the value to the corresponding screen height

	double HeightFloat = (double)TargetValue * (double)RendererCvt_CoordBufferSize.Y / (double)ValueMax;
	int16_t FloorHeight = (int16_t)HeightFloat;

	// Generate VT sequence

	// Fill unused cells

	int16_t TargetConsoleCol = (int16_t)iPosition;
	if (TargetConsoleCol >= RendererCvt_CoordBufferSize.X)
		TargetConsoleCol = RendererCvt_CoordBufferSize.X - 1;

	{
		// Update cell cache

		intptr_t i;
		for (i = 0; i < (intptr_t)(RendererCvt_CoordBufferSize.Y - FloorHeight); ++i) {

			RendererCvt_VtFormat Format = RendererCvt_AvAttrToVtFormat(Visualizer_MarkerAttribute_Background);
			RendererCvt_BufferCell* pbcCell = &RendererCvt_aBufferCellCache2D[RendererCvt_CoordBufferSize.X * i + TargetConsoleCol];
			pbcCell->Format.SgrForeground = Format.SgrForeground;
			pbcCell->Format.SgrBackground = Format.SgrBackground;

		}

		for (i; i < RendererCvt_CoordBufferSize.Y; ++i) {

			RendererCvt_VtFormat Format = RendererCvt_AvAttrToVtFormat(TargetAttr);
			RendererCvt_BufferCell* pbcCell = &RendererCvt_aBufferCellCache2D[RendererCvt_CoordBufferSize.X * i + TargetConsoleCol];
			pbcCell->Format.SgrForeground = Format.SgrForeground;
			pbcCell->Format.SgrBackground = Format.SgrBackground;

		}

	}

	// Count

	const char strFormat[] = "\x1b[%"PRIi16"G\x1b[%"PRIi16"d\x1b[%"PRIu8";%"PRIu8"m%s";
	intptr_t Length = 0;

	for (intptr_t i = 0; i < RendererCvt_CoordBufferSize.Y; ++i) {

		RendererCvt_BufferCell* pbcCell = &RendererCvt_aBufferCellCache2D[RendererCvt_CoordBufferSize.X * i + TargetConsoleCol];

		Length += (intptr_t)snprintf(
			NULL,
			0,
			strFormat,
			(int16_t)TargetConsoleCol + 1,
			(int16_t)i + 1,
			pbcCell->Format.SgrForeground,
			pbcCell->Format.SgrBackground,
			RendererCvt_GetCellCacheChar(pbcCell)
		);

	}
	Length += 1; // '\0'

	// Write to buffer

	char* strBuffer = malloc_guarded(Length * sizeof(char));
	char* strBufferCurrent = strBuffer;
	intptr_t LengthCurrent = Length;

	for (intptr_t i = 0; i < RendererCvt_CoordBufferSize.Y; ++i) {
		
		RendererCvt_BufferCell* pbcCell = &RendererCvt_aBufferCellCache2D[RendererCvt_CoordBufferSize.X * i + TargetConsoleCol];

		int Written = sprintf_s(
			strBufferCurrent,
			LengthCurrent,
			strFormat,
			(int16_t)TargetConsoleCol + 1,
			(int16_t)i + 1,
			pbcCell->Format.SgrForeground,
			pbcCell->Format.SgrBackground,
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
