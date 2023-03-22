
#include "Visualizer.h"
#include "GuardedMalloc.h"

#ifndef VISUALIZER_DISABLED

// Buffer stuff
static HANDLE hRendererBuffer = NULL;
static CONSOLE_SCREEN_BUFFER_INFOEX csbiRenderer = { 0 };

// Console Attr
// cmd "color /?" explains this very well.
#define ATTR_WINCON_BACKGROUND 0x0FU
#define ATTR_WINCON_NORMAL     0xF0U
#define ATTR_WINCON_READ       0x10U
#define ATTR_WINCON_WRITE      0x40U
#define ATTR_WINCON_POINTER    0x30U
#define ATTR_WINCON_CORRECT    0x40U
#define ATTR_WINCON_INCORRECT  0x20U

// For uninitialization
static ULONG OldInputMode = 0;
static HANDLE hOldBuffer = NULL;
static LONG_PTR OldWindowStyle = 0;

// Array
typedef struct {

	AV_ARRAYPROP_RENDERER vapr;

	// TODO: Horizontal scaling

} RWCC_ARRAYPROP;

static RWCC_ARRAYPROP RendererWcc_aRwccArrayProp[AV_MAX_ARRAY_COUNT];
// TODO: Linked list to keep track of active (added) items.

void RendererWcc_Initialize() {
	//

	HWND hWindow = GetConsoleWindow();
	OldWindowStyle = GetWindowLongPtrW(hWindow, GWL_STYLE);
	SetWindowLongPtrW(hWindow, GWL_STYLE, OldWindowStyle & ~WS_MAXIMIZEBOX & ~WS_SIZEBOX);
	SetWindowPos(hWindow, NULL, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOSIZE);

	//

	hRendererBuffer = WinConsole_CreateBuffer();

	//

	hOldBuffer = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleActiveScreenBuffer(hRendererBuffer);

	// Set console IO mode to RAW.

	GetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), &OldInputMode);
	SetConsoleMode(hRendererBuffer, 0);
	SetConsoleMode(hRendererBuffer, 0);

	// Set cursor to top left

	csbiRenderer.cbSize = sizeof(csbiRenderer);
	GetConsoleScreenBufferInfoEx(hRendererBuffer, &csbiRenderer);
	csbiRenderer.dwCursorPosition = (COORD){ 0, 0 };
	csbiRenderer.wAttributes = ATTR_WINCON_BACKGROUND;
	SetConsoleScreenBufferInfoEx(hRendererBuffer, &csbiRenderer);

	GetConsoleScreenBufferInfoEx(hRendererBuffer, &csbiRenderer);

	//

	WinConsole_Clear(hRendererBuffer);

	return;
}

void RendererWcc_Uninitialize() {

	SetConsoleActiveScreenBuffer(hOldBuffer);
	WinConsole_FreeBuffer(hRendererBuffer);

	SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), 0);

	HWND hWindow = GetConsoleWindow();
	SetWindowLongPtrW(hWindow, GWL_STYLE, OldWindowStyle);
	SetWindowPos(hWindow, NULL, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOSIZE);

}




void RendererWcc_AddArray(intptr_t ArrayId, intptr_t Size) {

	RendererWcc_aRwccArrayProp[ArrayId].vapr.Size = Size;
	RendererWcc_aRwccArrayProp[ArrayId].vapr.aArrayState = malloc_guarded(Size * sizeof(isort_t));
	RendererWcc_aRwccArrayProp[ArrayId].vapr.aAttribute = malloc_guarded(Size * sizeof(AvAttribute));

	RendererWcc_aRwccArrayProp[ArrayId].vapr.bVisible = FALSE;
	RendererWcc_aRwccArrayProp[ArrayId].vapr.ValueMin = 0;
	RendererWcc_aRwccArrayProp[ArrayId].vapr.ValueMax = 1;

	// Initialize arrays

	for (intptr_t i = 0; i < Size; ++i)
		RendererWcc_aRwccArrayProp[ArrayId].vapr.aArrayState[i] = 0;

	for (intptr_t i = 0; i < Size; ++i)
		RendererWcc_aRwccArrayProp[ArrayId].vapr.aAttribute[i] = AvAttribute_Normal;

	return;
}

void RendererWcc_RemoveArray(intptr_t ArrayId) {

	free(RendererWcc_aRwccArrayProp[ArrayId].vapr.aAttribute);
	free(RendererWcc_aRwccArrayProp[ArrayId].vapr.aArrayState);
	return;

}

void RendererWcc_UpdateArray(intptr_t ArrayId, isort_t NewSize, isort_t* aNewArrayState, int32_t bVisible, isort_t ValueMin, isort_t ValueMax) {

	RendererWcc_aRwccArrayProp[ArrayId].vapr.bVisible = bVisible;
	RendererWcc_aRwccArrayProp[ArrayId].vapr.ValueMin = ValueMin;
	RendererWcc_aRwccArrayProp[ArrayId].vapr.ValueMax = ValueMax;

	// Handle array resize

	if ((NewSize > 0) && (NewSize != RendererWcc_aRwccArrayProp[ArrayId].vapr.Size)) {

		// Realloc arrays

		isort_t* aResizedArrayState = realloc_guarded(
			RendererWcc_aRwccArrayProp[ArrayId].vapr.aArrayState,
			NewSize * sizeof(isort_t)
		);

		AvAttribute* aResizedAttribute = realloc_guarded(
			RendererWcc_aRwccArrayProp[ArrayId].vapr.aAttribute,
			NewSize * sizeof(AvAttribute)
		);


		intptr_t OldSize = RendererWcc_aRwccArrayProp[ArrayId].vapr.Size;
		intptr_t NewPartSize = NewSize - OldSize;

		// Initialize the new part

		for (intptr_t i = 0; i < NewPartSize; ++i)
			aResizedArrayState[OldSize + i] = 0;

		for (intptr_t i = 0; i < NewPartSize; ++i)
			aResizedAttribute[OldSize + i] = AvAttribute_Normal;

		RendererWcc_aRwccArrayProp[ArrayId].vapr.aArrayState = aResizedArrayState;
		RendererWcc_aRwccArrayProp[ArrayId].vapr.aAttribute = aResizedAttribute;

		RendererWcc_aRwccArrayProp[ArrayId].vapr.Size = NewSize;

	}

	isort_t* aArrayState = RendererWcc_aRwccArrayProp[ArrayId].vapr.aArrayState;
	intptr_t Size = RendererWcc_aRwccArrayProp[ArrayId].vapr.Size;

	// Handle new array state

	if (aNewArrayState)
		for (intptr_t i = 0; i < Size; ++i)
			aArrayState[i] = aNewArrayState[i];

	// Re-render with new props

	// Clear screen
	for (intptr_t i = 0; i < Size; ++i) {

		RendererWcc_UpdateItem(
			ArrayId,
			i,
			AV_RENDERER_UPDATEVALUE,
			0,
			0
		);

	}

	// Re-render using the same attribute
	if (aNewArrayState) {

		for (intptr_t i = 0; i < Size; ++i) {
			RendererWcc_UpdateItem(
				ArrayId,
				i,
				AV_RENDERER_UPDATEVALUE,
				aNewArrayState[i],
				0
			);
		}

	} else {

		for (intptr_t i = 0; i < Size; ++i) {
			RendererWcc_UpdateItem(
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

static const USHORT RendererWcc_WinConAttrTable[256] = {
	ATTR_WINCON_BACKGROUND, //AvAttribute_Background
	ATTR_WINCON_NORMAL,     //AvAttribute_Normal
	ATTR_WINCON_READ,       //AvAttribute_Read
	ATTR_WINCON_WRITE,      //AvAttribute_Write
	ATTR_WINCON_POINTER,    //AvAttribute_Pointer
	ATTR_WINCON_CORRECT,    //AvAttribute_Correct
	ATTR_WINCON_INCORRECT,  //AvAttribute_Incorrect
}; // 0: black background and black text.

static USHORT RendererWcc_AttrToConAttr(AvAttribute Attr) {
	return RendererWcc_WinConAttrTable[Attr]; // return 0 on unknown Attr.
}

void RendererWcc_UpdateItem(
	intptr_t ArrayId,
	uintptr_t iPos,
	uint32_t UpdateRequest,
	isort_t NewValue,
	AvAttribute NewAttr
) {

	// Choose the correct value & attribute

	isort_t TargetValue = RendererWcc_aRwccArrayProp[ArrayId].vapr.aArrayState[iPos];
	if (UpdateRequest & AV_RENDERER_UPDATEVALUE)
		TargetValue = NewValue;

	AvAttribute TargetAttr = RendererWcc_aRwccArrayProp[ArrayId].vapr.aAttribute[iPos];
	if (UpdateRequest & AV_RENDERER_UPDATEATTR)
		TargetAttr = NewAttr;

	RendererWcc_aRwccArrayProp[ArrayId].vapr.aArrayState[iPos] = TargetValue;
	RendererWcc_aRwccArrayProp[ArrayId].vapr.aAttribute[iPos] = TargetAttr;

	isort_t ValueMin = RendererWcc_aRwccArrayProp[ArrayId].vapr.ValueMin;
	isort_t ValueMax = RendererWcc_aRwccArrayProp[ArrayId].vapr.ValueMax;

	TargetValue -= ValueMin;
	ValueMax -= ValueMin; // Warning: Overflow

	if (TargetValue > ValueMax)
		TargetValue = ValueMax;

	// Scale the value to the corresponding screen height

	double dfHeight = (double)TargetValue * (double)csbiRenderer.dwSize.Y / (double)ValueMax;
	SHORT FloorHeight = (SHORT)dfHeight;

	// Convert AvAttribute to windows console attribute
	USHORT WinConAttr = RendererWcc_AttrToConAttr(TargetAttr);

	// Fill the unused cells with background.

	WinConsole_FillAttr(
		hRendererBuffer,
		&csbiRenderer,
		ATTR_WINCON_BACKGROUND,
		1,
		csbiRenderer.dwSize.Y - FloorHeight,
		(COORD){ (SHORT)iPos, 0 }
	);

	// Fill the used cells with WinConAttr.

	WinConsole_FillAttr(
		hRendererBuffer,
		&csbiRenderer,
		WinConAttr,
		1,
		FloorHeight,
		(COORD){ (SHORT)iPos, csbiRenderer.dwSize.Y - FloorHeight }
	);

	return;
}

#else
#endif
