
#include "Visualizer/Visualizer.h"
#include "Utils/GuardedMalloc.h"

#include "Visualizer/API/WindowsConsole.h"
#include "Visualizer/Renderer/ColumnWindowsConsole.h"

// Buffer stuff
static HANDLE hRendererAltBuffer = NULL;
static CONSOLE_SCREEN_BUFFER_INFOEX csbiRenderer = { 0 };

// Console Attr
// cmd "color /?" explains this very well.
#define ATTR_WINCON_BACKGROUND 0x0FU
#define ATTR_WINCON_NORMAL     0x78U
#define ATTR_WINCON_READ       0x1EU
#define ATTR_WINCON_WRITE      0x4BU
#define ATTR_WINCON_POINTER    0x3CU
#define ATTR_WINCON_CORRECT    0x4BU
#define ATTR_WINCON_INCORRECT  0x2EU

// For uninitialization
static ULONG OldInputMode = 0;
static HANDLE hOldBuffer = NULL;
static LONG_PTR OldWindowStyle = 0;

// Array
typedef struct {

	AV_ARRAYPROP_RENDERER vapr;

	// TODO: Horizontal scaling

} RCWC_ARRAYPROP;

static RCWC_ARRAYPROP RendererCwc_aRcwcArrayProp[AV_MAX_ARRAY_COUNT];
// TODO: Linked list to keep track of active (added) items.

void RendererCwc_Initialize() {

	// New window style

	HWND hWindow = GetConsoleWindow();
	OldWindowStyle = GetWindowLongPtrW(hWindow, GWL_STYLE);
	SetWindowLongPtrW(
		hWindow,
		GWL_STYLE,
		OldWindowStyle & ~WS_MAXIMIZEBOX & ~WS_SIZEBOX
	);
	SetWindowPos(
		hWindow,
		NULL,
		0,
		0,
		0,
		0,
		SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOSIZE
	);

	// New buffer

	hRendererAltBuffer = WinConsole_CreateBuffer();
	hOldBuffer = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleActiveScreenBuffer(hRendererAltBuffer);

	// Enable virtual terminal on Windows.

	GetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), &OldInputMode);
	SetConsoleMode(
		GetStdHandle(STD_INPUT_HANDLE),
		ENABLE_PROCESSED_INPUT | ENABLE_VIRTUAL_TERMINAL_INPUT
	);
	SetConsoleMode(
		hRendererAltBuffer,
		ENABLE_PROCESSED_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING
	);

	// Set cursor to top left

	csbiRenderer.cbSize = sizeof(csbiRenderer);
	GetConsoleScreenBufferInfoEx(hRendererAltBuffer, &csbiRenderer);
	csbiRenderer.dwCursorPosition = (COORD){ 0, 0 };
	csbiRenderer.wAttributes = ATTR_WINCON_BACKGROUND;
	SetConsoleScreenBufferInfoEx(hRendererAltBuffer, &csbiRenderer);

	GetConsoleScreenBufferInfoEx(hRendererAltBuffer, &csbiRenderer);

	//

	WinConsole_Clear(hRendererAltBuffer);

	return;
}

void RendererCwc_Uninitialize() {

	// Restore old console input mode

	SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), OldInputMode);

	// Free alternate buffer

	SetConsoleActiveScreenBuffer(hOldBuffer);
	WinConsole_FreeBuffer(hRendererAltBuffer);

	// Restore window mode

	HWND hWindow = GetConsoleWindow();
	SetWindowLongPtrW(hWindow, GWL_STYLE, OldWindowStyle);
	SetWindowPos(hWindow, NULL, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOSIZE);

	return;

}




void RendererCwc_AddArray(intptr_t ArrayId, intptr_t Size) {

	RendererCwc_aRcwcArrayProp[ArrayId].vapr.Size = Size;
	RendererCwc_aRcwcArrayProp[ArrayId].vapr.aArrayState = malloc_guarded(Size * sizeof(isort_t));
	RendererCwc_aRcwcArrayProp[ArrayId].vapr.aAttribute = malloc_guarded(Size * sizeof(AvAttribute));

	RendererCwc_aRcwcArrayProp[ArrayId].vapr.bVisible = FALSE;
	RendererCwc_aRcwcArrayProp[ArrayId].vapr.ValueMin = 0;
	RendererCwc_aRcwcArrayProp[ArrayId].vapr.ValueMax = 1;

	// Initialize arrays

	for (intptr_t i = 0; i < Size; ++i)
		RendererCwc_aRcwcArrayProp[ArrayId].vapr.aArrayState[i] = 0;

	for (intptr_t i = 0; i < Size; ++i)
		RendererCwc_aRcwcArrayProp[ArrayId].vapr.aAttribute[i] = AvAttribute_Normal;

	return;

}

void RendererCwc_RemoveArray(intptr_t ArrayId) {

	free(RendererCwc_aRcwcArrayProp[ArrayId].vapr.aAttribute);
	free(RendererCwc_aRcwcArrayProp[ArrayId].vapr.aArrayState);
	return;

}

void RendererCwc_UpdateArray(intptr_t ArrayId, isort_t NewSize, isort_t* aNewArrayState, bool bVisible, isort_t ValueMin, isort_t ValueMax) {
		
	// Clear screen
	
	for (intptr_t i = 0; i < RendererCwc_aRcwcArrayProp[ArrayId].vapr.Size; ++i) {

		RendererCwc_UpdateItem(
			ArrayId,
			i,
			AV_RENDERER_UPDATEVALUE,
			0,
			0
		);

	}

	RendererCwc_aRcwcArrayProp[ArrayId].vapr.bVisible = bVisible;
	RendererCwc_aRcwcArrayProp[ArrayId].vapr.ValueMin = ValueMin;
	RendererCwc_aRcwcArrayProp[ArrayId].vapr.ValueMax = ValueMax;

	// Handle array resize

	if ((NewSize > 0) && (NewSize != RendererCwc_aRcwcArrayProp[ArrayId].vapr.Size)) {

		// Realloc arrays

		isort_t* aResizedArrayState = realloc_guarded(
			RendererCwc_aRcwcArrayProp[ArrayId].vapr.aArrayState,
			NewSize * sizeof(isort_t)
		);

		AvAttribute* aResizedAttribute = realloc_guarded(
			RendererCwc_aRcwcArrayProp[ArrayId].vapr.aAttribute,
			NewSize * sizeof(AvAttribute)
		);


		intptr_t OldSize = RendererCwc_aRcwcArrayProp[ArrayId].vapr.Size;
		intptr_t NewPartSize = NewSize - OldSize;

		// Initialize the new part

		for (intptr_t i = 0; i < NewPartSize; ++i)
			aResizedArrayState[OldSize + i] = 0;

		for (intptr_t i = 0; i < NewPartSize; ++i)
			aResizedAttribute[OldSize + i] = AvAttribute_Normal;

		RendererCwc_aRcwcArrayProp[ArrayId].vapr.aArrayState = aResizedArrayState;
		RendererCwc_aRcwcArrayProp[ArrayId].vapr.aAttribute = aResizedAttribute;

		RendererCwc_aRcwcArrayProp[ArrayId].vapr.Size = NewSize;

	}

	isort_t* aArrayState = RendererCwc_aRcwcArrayProp[ArrayId].vapr.aArrayState;
	intptr_t Size = RendererCwc_aRcwcArrayProp[ArrayId].vapr.Size;

	// Handle new array state

	if (aNewArrayState)
		for (intptr_t i = 0; i < Size; ++i)
			aArrayState[i] = aNewArrayState[i];

	// Re-render with new props

	// Same attribute
	if (aNewArrayState) {

		for (intptr_t i = 0; i < Size; ++i) {
			RendererCwc_UpdateItem(
				ArrayId,
				i,
				AV_RENDERER_UPDATEVALUE,
				aNewArrayState[i],
				0
			);
		}

	} else {

		for (intptr_t i = 0; i < Size; ++i) {
			RendererCwc_UpdateItem(
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

static const USHORT RendererCwc_WinConAttrTable[256] = {
	ATTR_WINCON_BACKGROUND, //AvAttribute_Background
	ATTR_WINCON_NORMAL,     //AvAttribute_Normal
	ATTR_WINCON_READ,       //AvAttribute_Read
	ATTR_WINCON_WRITE,      //AvAttribute_Write
	ATTR_WINCON_POINTER,    //AvAttribute_Pointer
	ATTR_WINCON_CORRECT,    //AvAttribute_Correct
	ATTR_WINCON_INCORRECT,  //AvAttribute_Incorrect
}; // 0: black background and black text.

static USHORT RendererCwc_AttrToConAttr(AvAttribute Attr) {
	return RendererCwc_WinConAttrTable[Attr]; // return 0 on unknown Attr.
}

void RendererCwc_UpdateItem(
	intptr_t ArrayId,
	uintptr_t iPos,
	uint32_t UpdateRequest,
	isort_t NewValue,
	AvAttribute NewAttr
) {

	// Choose the correct value & attribute

	isort_t TargetValue = RendererCwc_aRcwcArrayProp[ArrayId].vapr.aArrayState[iPos];
	if (UpdateRequest & AV_RENDERER_UPDATEVALUE)
		TargetValue = NewValue;

	AvAttribute TargetAttr = RendererCwc_aRcwcArrayProp[ArrayId].vapr.aAttribute[iPos];
	if (UpdateRequest & AV_RENDERER_UPDATEATTR)
		TargetAttr = NewAttr;

	RendererCwc_aRcwcArrayProp[ArrayId].vapr.aArrayState[iPos] = TargetValue;
	RendererCwc_aRcwcArrayProp[ArrayId].vapr.aAttribute[iPos] = TargetAttr;

	isort_t ValueMin = RendererCwc_aRcwcArrayProp[ArrayId].vapr.ValueMin;
	isort_t ValueMax = RendererCwc_aRcwcArrayProp[ArrayId].vapr.ValueMax;

	TargetValue -= ValueMin;
	ValueMax -= ValueMin; // Warning: Overflow

	if (TargetValue > ValueMax)
		TargetValue = ValueMax;

	// Scale the value to the corresponding screen height

	double dfHeight = (double)TargetValue * (double)csbiRenderer.dwSize.Y / (double)ValueMax;
	SHORT FloorHeight = (SHORT)dfHeight;

	// Convert AvAttribute to windows console attribute
	USHORT WinConAttr = RendererCwc_AttrToConAttr(TargetAttr);

	// Fill the unused cells with background.

	WinConsole_FillAttr(
		hRendererAltBuffer,
		&csbiRenderer,
		ATTR_WINCON_BACKGROUND,
		1,
		csbiRenderer.dwSize.Y - FloorHeight,
		(COORD){ (SHORT)iPos, 0 }
	);

	// Fill the used cells with WinConAttr.

	WinConsole_FillAttr(
		hRendererAltBuffer,
		&csbiRenderer,
		WinConAttr,
		1,
		FloorHeight,
		(COORD){ (SHORT)iPos, csbiRenderer.dwSize.Y - FloorHeight }
	);

	return;

}
