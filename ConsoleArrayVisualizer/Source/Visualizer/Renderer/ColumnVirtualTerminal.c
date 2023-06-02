
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

static tree234* RendererCvt_ptreeGlobalArrayProp; // tree of RCVT_ARRAYPROP_RENDERER

static int RendererCvt_ArrayPropIdCmp(void* pA, void* pB) {
	RCVT_ARRAYPROP* pvapA = pA;
	RCVT_ARRAYPROP* pvapB = pB;
	return (pvapA->vapr.ArrayId > pvapB->vapr.ArrayId) - (pvapA->vapr.ArrayId < pvapB->vapr.ArrayId);
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
	bool bBold         : 4; // ON / OFF
	bool bUnderline    : 4; // ON / OFF
	bool bNegative     : 4; // ON / OFF
	bool bMbText       : 4; // YES / NO

	// MbText is only used when writing text and is updated automatically.
	// https://devblogs.microsoft.com/commandline/windows-command-line-unicode-and-utf-8-output-text-buffer/

} RCVT_VTFORMAT;

typedef struct {

	// UTF-32 data
	uint32_t Char;

	RCVT_VTFORMAT vtfFormat;

} RCVT_BUFFER_CELL;

static RCVT_COORD RendererCvt_coordBufferSize;
static RCVT_BUFFER_CELL* RendererCvt_abcBufferCellCache;

static RCVT_VTFORMAT RendererCvt_AvAttrToVtFormat(AvAttribute Attribute) {
	RCVT_VTFORMAT VtFormatTable[32] = { 0 };
	VtFormatTable[AvAttribute_Background] = (RCVT_VTFORMAT){ 97, 40, false, false, false, false };
	VtFormatTable[AvAttribute_Normal    ] = (RCVT_VTFORMAT){ 90, 47, false, false, false, false };
	VtFormatTable[AvAttribute_Read      ] = (RCVT_VTFORMAT){ 93, 44, false, false, false, false };
	VtFormatTable[AvAttribute_Write     ] = (RCVT_VTFORMAT){ 96, 41, false, false, false, false };
	VtFormatTable[AvAttribute_Pointer   ] = (RCVT_VTFORMAT){ 91, 46, false, false, false, false };
	VtFormatTable[AvAttribute_Correct   ] = (RCVT_VTFORMAT){ 95, 42, false, false, false, false };
	VtFormatTable[AvAttribute_Incorrect ] = (RCVT_VTFORMAT){ 96, 41, false, false, false, false };
	return ((unsigned int)Attribute < 32) ? VtFormatTable[Attribute] : (RCVT_VTFORMAT){ 0, 0, false, false, false, false }; // return 0 on unknown Attr.
}

typedef struct {

	// true means do update, false means keep intact.
	bool bForeground : 8;
	bool bBackground : 8;
	bool bBold       : 4;
	bool bUnderline  : 4;
	bool bNegative   : 4;
	bool bText       : 4;

} RCVT_VTFORMAT_UPDATEREQUEST;

#define RendererCvt_GenerateVtSequence_WriteGuard(bWrite, s, c) { \
	if (bWrite) *(s) = (c); \
	++(s); \
}

// Returns the position after the last digit
static intptr_t RendererCvt_i16toa(bool bWrite, int16_t X, char* s) {

	char* sCurrent = s;

	if (X < 0)
		RendererCvt_GenerateVtSequence_WriteGuard(bWrite, sCurrent, '-'); // +1

	if (X > 0) X = -X;
	if (X <= -10000)
		RendererCvt_GenerateVtSequence_WriteGuard(bWrite, sCurrent, -(X / 10000 % 10) + '0'); // +1
	if (X <= -1000)
		RendererCvt_GenerateVtSequence_WriteGuard(bWrite, sCurrent, -(X / 1000 % 10) + '0'); // +1
	if (X <= -100)
		RendererCvt_GenerateVtSequence_WriteGuard(bWrite, sCurrent, -(X / 100 % 10) + '0'); // +1
	if (X <= -10)
		RendererCvt_GenerateVtSequence_WriteGuard(bWrite, sCurrent, -(X / 10 % 10) + '0'); // +1
	if (X <= 0)
		RendererCvt_GenerateVtSequence_WriteGuard(bWrite, sCurrent, -(X % 10) + '0'); // +1

	return sCurrent - s;

}

// Per cell
// Assume sBuffer is large enough (27 chars)
static intptr_t RendererCvt_GenerateVtSequence(
	bool bWrite,
	char* sBuffer,
	RCVT_VTFORMAT_UPDATEREQUEST vtfurRequest,
	RCVT_BUFFER_CELL* pbcCell
) {

	char* sBegin = sBuffer;
	char* sCurrent = sBuffer;

	// This function is designed to reduce the number of VT sequences.

	RendererCvt_GenerateVtSequence_WriteGuard(bWrite, sCurrent, '\x1B'); // +1
	RendererCvt_GenerateVtSequence_WriteGuard(bWrite, sCurrent, '['); // +1

	bool bNeedSemicolon = false;
	if (vtfurRequest.bForeground) {
		sCurrent += RendererCvt_i16toa(bWrite, (int16_t)pbcCell->vtfFormat.sgrForeground, sCurrent); // +6
		bNeedSemicolon = true;
	}

	if (vtfurRequest.bBackground) {
		if (bNeedSemicolon) {
			RendererCvt_GenerateVtSequence_WriteGuard(bWrite, sCurrent, ';'); // +1
		}
		sCurrent += RendererCvt_i16toa(bWrite, (int16_t)pbcCell->vtfFormat.sgrBackground, sCurrent); // +6
		bNeedSemicolon = true;
	}

	if (vtfurRequest.bBold) {
		if (bNeedSemicolon) {
			RendererCvt_GenerateVtSequence_WriteGuard(bWrite, sCurrent, ';'); // +1
		}
		if (pbcCell->vtfFormat.bBold) {
			RendererCvt_GenerateVtSequence_WriteGuard(bWrite, sCurrent, '1'); // +1
		} else {
			RendererCvt_GenerateVtSequence_WriteGuard(bWrite, sCurrent, '2'); // +1
			RendererCvt_GenerateVtSequence_WriteGuard(bWrite, sCurrent, '2'); // +1
		}
		bNeedSemicolon = true;
	}

	if (vtfurRequest.bUnderline) {
		if (bNeedSemicolon) {
			RendererCvt_GenerateVtSequence_WriteGuard(bWrite, sCurrent, ';'); // +1
		}
		if (pbcCell->vtfFormat.bUnderline) {
			RendererCvt_GenerateVtSequence_WriteGuard(bWrite, sCurrent, '4'); // +1
		} else {
			RendererCvt_GenerateVtSequence_WriteGuard(bWrite, sCurrent, '2'); // +1
			RendererCvt_GenerateVtSequence_WriteGuard(bWrite, sCurrent, '4'); // +1
		}
		bNeedSemicolon = true;
	}

	if (vtfurRequest.bNegative) {
		if (bNeedSemicolon) {
			RendererCvt_GenerateVtSequence_WriteGuard(bWrite, sCurrent, ';'); // +1
		}
		if (pbcCell->vtfFormat.bNegative) {
			RendererCvt_GenerateVtSequence_WriteGuard(bWrite, sCurrent, '7'); // +1
		} else {
			RendererCvt_GenerateVtSequence_WriteGuard(bWrite, sCurrent, '2'); // +1
			RendererCvt_GenerateVtSequence_WriteGuard(bWrite, sCurrent, '7'); // +1
		}
		bNeedSemicolon = true;
	}

	RendererCvt_GenerateVtSequence_WriteGuard(bWrite, sCurrent, 'm'); // +1

	if (vtfurRequest.bText) {
		RendererCvt_GenerateVtSequence_WriteGuard(bWrite, sCurrent, (char)pbcCell->Char); // +1
		// TODO: Multi-byte unicode text
	}

	return sCurrent - sBegin;

}

static void RendererCvt_UpdateCellCache(
	RCVT_COORD coordCell,
	RCVT_VTFORMAT_UPDATEREQUEST vtfurRequest,
	RCVT_VTFORMAT vtfFormat,
	uint32_t* aCharCP,
	int32_t nCP
) {

	if (coordCell.Y >= RendererCvt_coordBufferSize.Y || coordCell.X >= RendererCvt_coordBufferSize.X)
		abort();

	int32_t iIndex1D = RendererCvt_coordBufferSize.X * coordCell.Y + coordCell.X;

	if (vtfurRequest.bForeground)
		RendererCvt_abcBufferCellCache[iIndex1D].vtfFormat.sgrForeground = vtfFormat.sgrForeground;
	if (vtfurRequest.bBackground)
		RendererCvt_abcBufferCellCache[iIndex1D].vtfFormat.sgrBackground = vtfFormat.sgrBackground;

	if (vtfurRequest.bBold)
		RendererCvt_abcBufferCellCache[iIndex1D].vtfFormat.bBold = vtfFormat.bBold;
	if (vtfurRequest.bUnderline)
		RendererCvt_abcBufferCellCache[iIndex1D].vtfFormat.bUnderline = vtfFormat.bUnderline;
	if (vtfurRequest.bNegative)
		RendererCvt_abcBufferCellCache[iIndex1D].vtfFormat.bNegative = vtfFormat.bNegative;

	if (vtfurRequest.bText) {
		// TODO: Multi-byte unicode text
		RendererCvt_abcBufferCellCache[iIndex1D].Char = aCharCP[0];
	}

	return;

}

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
			' ',
			RendererCvt_AvAttrToVtFormat(AvAttribute_Background)
		};
		RCVT_VTFORMAT_UPDATEREQUEST vtfurRequest = {
			.bForeground = true,
			.bBackground = true,
			.bBold       = true, // OFF by default
			.bUnderline  = true, // OFF by default
			.bNegative   = true, // OFF by default
			.bText       = false // VT100 haves a sequence for this
		};
		
		// Count
		intptr_t Length = RendererCvt_GenerateVtSequence(
			false,
			NULL,
			vtfurRequest,
			&bcCell
		);
		
		// Allocate
		char* sBuffer = malloc_guarded((Length + 4) * sizeof(char));
		char* pBufferCurrent = sBuffer;

		pBufferCurrent += RendererCvt_GenerateVtSequence(
			false,
			pBufferCurrent,
			vtfurRequest,
			&bcCell
		);

		// Clear screen sequence

		*pBufferCurrent++ = '\x1B'; // +1
		*pBufferCurrent++ = '['; // +1
		*pBufferCurrent++ = '2'; // +1
		*pBufferCurrent++ = 'K'; // +1

		fwrite(sBuffer, sizeof(char), pBufferCurrent - sBuffer, stdout);
		

		// Update cell cache

		for (intptr_t i = 0; i < BufferSize1D; ++i) {
			RendererCvt_abcBufferCellCache[i].Char = (uint32_t)' ';
			RendererCvt_abcBufferCellCache[i].vtfFormat = RendererCvt_AvAttrToVtFormat(AvAttribute_Background);
		}
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

	prapArrayProp->vapr.ArrayId = ArrayId;
	prapArrayProp->vapr.Size = Size;

	prapArrayProp->vapr.aArrayState = malloc_guarded(Size * sizeof(isort_t));
	for (intptr_t i = 0; i < Size; ++i)
		prapArrayProp->vapr.aArrayState[i] = 0;

	prapArrayProp->vapr.aAttribute = malloc_guarded(Size * sizeof(AvAttribute));
	for (intptr_t i = 0; i < Size; ++i)
		prapArrayProp->vapr.aAttribute[i] = AvAttribute_Normal;

	prapArrayProp->vapr.bVisible = false;
	prapArrayProp->vapr.ValueMin = 0;
	prapArrayProp->vapr.ValueMax = 1;

	// Add to tree

	add234(RendererCvt_ptreeGlobalArrayProp, prapArrayProp);

	return;
}

void RendererCvt_RemoveArray(intptr_t ArrayId) {

	RCVT_ARRAYPROP rapFind = { .vapr.ArrayId = ArrayId };
	RCVT_ARRAYPROP* prapArrayProp = find234(RendererCvt_ptreeGlobalArrayProp, &rapFind, NULL);

	free(prapArrayProp->vapr.aAttribute);
	free(prapArrayProp->vapr.aArrayState);

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

	RCVT_ARRAYPROP rapFind = { .vapr.ArrayId = ArrayId };
	RCVT_ARRAYPROP* prapArrayProp = find234(RendererCvt_ptreeGlobalArrayProp, &rapFind, NULL);
	
	// Clear screen
	
	for (intptr_t i = 0; i < prapArrayProp->vapr.Size; ++i) {

		RendererCvt_UpdateItem(
			ArrayId,
			i,
			AV_RENDERER_UPDATEVALUE,
			0,
			0
		);

	}

	prapArrayProp->vapr.bVisible = bVisible;
	prapArrayProp->vapr.ValueMin = ValueMin;
	prapArrayProp->vapr.ValueMax = ValueMax;

	// Handle array resize

	if ((NewSize > 0) && (NewSize != prapArrayProp->vapr.Size)) {

		// Realloc arrays

		isort_t* aResizedArrayState = realloc_guarded(
			prapArrayProp->vapr.aArrayState,
			NewSize * sizeof(isort_t)
		);

		AvAttribute* aResizedAttribute = realloc_guarded(
			prapArrayProp->vapr.aAttribute,
			NewSize * sizeof(AvAttribute)
		);


		intptr_t OldSize = prapArrayProp->vapr.Size;
		intptr_t NewPartSize = NewSize - OldSize;

		// Initialize the new part

		for (intptr_t i = 0; i < NewPartSize; ++i)
			aResizedArrayState[OldSize + i] = 0;

		for (intptr_t i = 0; i < NewPartSize; ++i)
			aResizedAttribute[OldSize + i] = AvAttribute_Normal;

		prapArrayProp->vapr.aArrayState = aResizedArrayState;
		prapArrayProp->vapr.aAttribute = aResizedAttribute;

		prapArrayProp->vapr.Size = NewSize;

	}

	isort_t* aArrayState = prapArrayProp->vapr.aArrayState;
	intptr_t Size = prapArrayProp->vapr.Size;

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

	RCVT_ARRAYPROP rapFind = { .vapr.ArrayId = ArrayId };
	RCVT_ARRAYPROP* prapArrayProp = find234(RendererCvt_ptreeGlobalArrayProp, &rapFind, NULL);

	// Choose the correct value & attribute

	isort_t TargetValue = prapArrayProp->vapr.aArrayState[iPos];
	if (UpdateRequest & AV_RENDERER_UPDATEVALUE)
		TargetValue = NewValue;

	AvAttribute TargetAttr = prapArrayProp->vapr.aAttribute[iPos];
	if (UpdateRequest & AV_RENDERER_UPDATEATTR)
		TargetAttr = NewAttr;

	prapArrayProp->vapr.aArrayState[iPos] = TargetValue;
	prapArrayProp->vapr.aAttribute[iPos] = TargetAttr;

	isort_t ValueMin = prapArrayProp->vapr.ValueMin;
	isort_t ValueMax = prapArrayProp->vapr.ValueMax;

	TargetValue -= ValueMin;
	ValueMax -= ValueMin; // Warning: Overflow

	if (TargetValue > ValueMax)
		TargetValue = ValueMax;

	// Scale the value to the corresponding screen height

	double dfHeight = (double)TargetValue * (double)RendererCvt_coordBufferSize.Y / (double)ValueMax;
	int16_t FloorHeight = (int)dfHeight;

	// Generate VT sequence

	// Fill unused cells

	int16_t TargetConsoleCol = (int16_t)iPos;
	if (TargetConsoleCol >= RendererCvt_coordBufferSize.X)
		TargetConsoleCol = RendererCvt_coordBufferSize.X - 1;

	// Count (copy & patse code) & update cache at the same time
	intptr_t Length = 0;
	{
		intptr_t i = 0;
		for (i; i < (intptr_t)(RendererCvt_coordBufferSize.Y - FloorHeight); ++i) {

			Length += RendererCvt_i16toa(false, (int16_t)TargetConsoleCol + 1, NULL);
			Length += RendererCvt_i16toa(false, (int16_t)i + 1, NULL);

			RCVT_VTFORMAT vtfFormat = RendererCvt_AvAttrToVtFormat(AvAttribute_Background);
			RCVT_VTFORMAT_UPDATEREQUEST vtfurRequest = {
				.bForeground = true,
				.bBackground = true,
				.bBold       = false,
				.bUnderline  = false,
				.bNegative   = false,
				.bText       = false
			};

			RendererCvt_UpdateCellCache(
				(RCVT_COORD){ TargetConsoleCol, (int16_t)i },
				vtfurRequest,
				vtfFormat,
				NULL,
				0
			);

			vtfurRequest.bText = true;
			Length += RendererCvt_GenerateVtSequence(
				false,
				NULL,
				vtfurRequest,
				&RendererCvt_abcBufferCellCache[RendererCvt_coordBufferSize.X * i + TargetConsoleCol]
			);

		}

		for (i; i < RendererCvt_coordBufferSize.Y; ++i) {

			Length += RendererCvt_i16toa(false, (int16_t)TargetConsoleCol + 1, NULL);
			Length += RendererCvt_i16toa(false, (int16_t)i + 1, NULL);

			RCVT_VTFORMAT vtfFormat = RendererCvt_AvAttrToVtFormat(TargetAttr);
			RCVT_VTFORMAT_UPDATEREQUEST vtfurRequest = {
				.bForeground = true,
				.bBackground = true,
				.bBold       = false,
				.bUnderline  = false,
				.bNegative   = false,
				.bText       = false
			};
		
			RendererCvt_UpdateCellCache(
				(RCVT_COORD){ TargetConsoleCol, (int16_t)i },
				vtfurRequest,
				vtfFormat,
				NULL,
				0
			);

			vtfurRequest.bText = true;
			Length += RendererCvt_GenerateVtSequence(
				false,
				NULL,
				vtfurRequest,
				&RendererCvt_abcBufferCellCache[RendererCvt_coordBufferSize.X * i + TargetConsoleCol]
			);

		}
	}

	// Generate VT sequence

	char* sBuffer = malloc_guarded((Length + ((size_t)6 * RendererCvt_coordBufferSize.Y)) * sizeof(char));
	char* pBufferCurrent = sBuffer;

	intptr_t i = 0;
	for (i; i < (intptr_t)(RendererCvt_coordBufferSize.Y - FloorHeight); ++i) {

		// Change horizontal pos

		*pBufferCurrent++ = '\x1B'; // +1
		*pBufferCurrent++ = '['; // +1
		pBufferCurrent += RendererCvt_i16toa(true, (int16_t)TargetConsoleCol + 1, pBufferCurrent);
		*pBufferCurrent++ = 'G'; // +1

		// Change vertical pos

		*pBufferCurrent++ = '\x1B'; // +1
		*pBufferCurrent++ = '['; // +1
		pBufferCurrent += RendererCvt_i16toa(true, (int16_t)i + 1, pBufferCurrent);
		*pBufferCurrent++ = 'd'; // +1

		// Change text color

		RCVT_VTFORMAT_UPDATEREQUEST vtfurRequest = {
			.bForeground = true,
			.bBackground = true,
			.bBold       = false,
			.bUnderline  = false,
			.bNegative   = false,
			.bText       = false
		};

		vtfurRequest.bText = true;
		pBufferCurrent += RendererCvt_GenerateVtSequence(
			true,
			pBufferCurrent,
			vtfurRequest,
			&RendererCvt_abcBufferCellCache[RendererCvt_coordBufferSize.X * i + TargetConsoleCol]
			// Cache is already updated so vtfFormat is.
		);

	}

	for (i; i < RendererCvt_coordBufferSize.Y; ++i) {

		// Change horizontal pos

		*pBufferCurrent++ = '\x1B'; // +1
		*pBufferCurrent++ = '['; // +1
		pBufferCurrent += RendererCvt_i16toa(true, (int16_t)TargetConsoleCol + 1, pBufferCurrent);
		*pBufferCurrent++ = 'G'; // +1

		// Change vertical pos

		*pBufferCurrent++ = '\x1B'; // +1
		*pBufferCurrent++ = '['; // +1
		pBufferCurrent += RendererCvt_i16toa(true, (int16_t)i + 1, pBufferCurrent);
		*pBufferCurrent++ = 'd'; // +1

		// Change text color

		RCVT_VTFORMAT_UPDATEREQUEST vtfurRequest = {
			.bForeground = true,
			.bBackground = true,
			.bBold       = false,
			.bUnderline  = false,
			.bNegative   = false,
			.bText       = false
		};

		vtfurRequest.bText = true;
		pBufferCurrent += RendererCvt_GenerateVtSequence(
			true,
			pBufferCurrent,
			vtfurRequest,
			&RendererCvt_abcBufferCellCache[RendererCvt_coordBufferSize.X * i + TargetConsoleCol]
			// Cache is already updated so vtfFormat is.
		);

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
