
#include <stddef.h>
#include <Windows.h>

void printf(char* fmtstr, ...);

typedef signed char SCHAR;

// First, count the length needed for the format process,
// Then allocate memory from the heap and do format.
// Print the result after all.
// The final result must be shorter than than ULONG_MAX (WriteConsoleA() limit).

ULONG HeapPrintF(const CHAR* FormatStr, ...) {

	va_list VaList;

	ULONG Length = 0;

	while (FormatStr[Length] != '\0') {
		if (Length == ULONG_MAX) {
			va_end(VaList);
			return 1;
		}
		Length += 1;
	}

	CHAR* Result = NULL; // NULL terminating is not necessary.

	ULONG ResultIndex = 0;

	HANDLE ProcHeap = GetProcessHeap();
	if ((ProcHeap == 0) || (ProcHeap == INVALID_HANDLE_VALUE)) {
		return 2;
	}

	UCHAR Write = FALSE;
	while (TRUE) {

		va_start(VaList, FormatStr);
		for (ULONG Index = 0; Index < Length; ++Index) {

			if (FormatStr[Index] != '%') {

				if (Write == TRUE) Result[ResultIndex] = FormatStr[Index];
				++ResultIndex;
				continue;

			} else {

				// "%%" then print "%".
				if (FormatStr[Index + 1] == '%') {
					if (Write == TRUE) Result[ResultIndex] = FormatStr[Index + 1];
					++ResultIndex;
					++Index;
					continue;
				}


				// Find type.
				UCHAR Prefix = 0;
				ULONG Type = 0;
				/*

				Prefixes:
				Signed int    :  0
				Unsigned int  : 'U'
				Uppercase Hex : 'X'
				Lowercase Hex : 'x'
				String / Char : 'S'

				Int types (Postfixes):
				8 bits int    : 'H' + 'H'
				16 bits int   : 'H'
				32 bits int   : 'L'
				64 bits int   : 'L' + 'L'


				*/
				const CHAR* TypeAdr = &(FormatStr[Index + 1]);

				// Skip digits.
				while ((*TypeAdr >= '0') && (*TypeAdr <= '9')) {
					++TypeAdr;
				}

				// Prefix check.
				if (*TypeAdr == 'S') {

					Prefix = 'S';          // String
					Type = (ULONG)('H' + 'H');
					++TypeAdr;
					// No postfixes for String type.

				} else {
					// If not String, then some integer type.

					if ((*TypeAdr == 'U') || (*TypeAdr == 'X') || (*TypeAdr == 'x')) {

						Prefix = *TypeAdr;
						++TypeAdr;

					} // else signed.

					// Postfix check.
					if (*TypeAdr == 'P') {         // Pointer

						if (Prefix == 0) Prefix = 'X';

						#ifdef _WIN64
						Type = (ULONG)('L' + 'L');
						#else
						Type = (ULONG)('L');
						#endif

						++TypeAdr;

					} else if (*TypeAdr == 'L') {  // 32 bits int

						Type = (ULONG)'L';
						++TypeAdr;
						if (*TypeAdr == 'L') {     // 64 bits int
							Type += (ULONG)'L';
							++TypeAdr;
						}

					} else if (*TypeAdr == 'H') {  // 16 bits int

						Type = (ULONG)'H';
						++TypeAdr;
						if (*TypeAdr == 'H') {     // 8 bits int
							Type += (ULONG)'H';
							++TypeAdr;
						}

					}
					if (Type == 0) {
						if (Prefix != 0) {
							Type = (ULONG)'L';      // 32 bits is the default.
						}
					}
				}
				--TypeAdr;

				if (Type == 0) continue;

				// Check for format width.
				ULONG ConvWidth = 0;

				{
					ULONG DigitNumber = 0;
					while ((FormatStr[Index + DigitNumber + 1] >= '0') && (FormatStr[Index + DigitNumber + 1] <= '9')) {
						DigitNumber += 1;
					}

					if (DigitNumber != 0) {
						// StrToUL()

						if (DigitNumber > 4) { // 2048 max string len.
							DigitNumber = 4;
						}
						ULONG Power = 1;
						ULONG DN2 = DigitNumber;
						ConvWidth = 0;

						while (DN2 > 0) {

							ConvWidth += (FormatStr[DN2 + Index] - '0') * Power;
							Power *= 10;

							DN2 -= 1;
						}
						Index += DigitNumber;

					}
				}
				// All specifier will not be displayed.
				Index += (ULONG)(TypeAdr - &(FormatStr[Index]));

				// Convert.
				if (Prefix == 'S') {

					CHAR* Str = va_arg(VaList, CHAR*);
					ULONG StrLength = ConvWidth;

					if (StrLength == 0) {
						while (Str[StrLength] != '\0') {
							if (Length == ULONG_MAX) {
								if (Write == TRUE) HeapFree(ProcHeap, HEAP_NO_SERIALIZE, Result);
								return 1;
							}
							StrLength += 1;
						}
					}

					if (Write == TRUE) {
						while (StrLength > 0) {
							Result[ResultIndex] = *Str;
							++ResultIndex;
							++Str;
							--StrLength;
						}
					} else {
						ResultIndex += StrLength;
					}

				} else {

					if (ConvWidth > 40) ConvWidth = 40;

					ULONG Base = 10;
					LONG IsNeg = FALSE;

					const CHAR* HexChars = "0123456789ABCDEF";

					if (Prefix == 'x') {
						HexChars = "0123456789abcdef";
						Base = 16;
					} else if (Prefix == 'X') {
						Base = 16;
					}

					if (Type == (ULONG)('L' + 'L')) {

						ULONGLONG Num;
						Num = va_arg(VaList, ULONGLONG);

						if (Prefix == 0) { // Signed

							if ((LONGLONG)Num < 0LL) {
								Num = ~Num + 1;
								IsNeg = TRUE;
							}
						}

						ULONG DigitsNum = 1;

						ULONGLONG Temp = Num;
						while (Temp > (Base - 1)) {
							Temp /= Base;
							DigitsNum += 1;
						}

						if (ConvWidth > DigitsNum) {
							DigitsNum = ConvWidth;
						}


						if (IsNeg) {
							if (Write == TRUE) Result[ResultIndex] = '-';
							++ResultIndex;
						}

						CHAR* ResultAdr = NULL;
						if (Write == TRUE) ResultAdr = &(Result[ResultIndex]);

						Temp = Num;
						ResultIndex += DigitsNum;

						if (Write == TRUE) {
							while (DigitsNum > 0) {
								ResultAdr[DigitsNum - 1] = HexChars[Temp % Base];
								Temp /= Base;
								DigitsNum -= 1;
							}
						}

					} else {

						ULONG Num;
						Num = va_arg(VaList, ULONG);


						if (Type == (ULONG)'H') {
							Num &= 0xFFFF;
						} else if (Type == (ULONG)('H' + 'H')) {
							Num &= 0xFF;
						}

						if (Prefix == 0) { // Signed

							if (Type == (ULONG)'H') {
								if ((SHORT)(Num) < 0) Num |= 0xFFFF0000;

							} else if (Type == (ULONG)('H' + 'H')) {
								if ((SCHAR)(Num) < 0) Num |= 0xFFFFFF00;
								break;
							}

							if ((LONG)Num < 0L) {
								Num = ~Num + 1;
								IsNeg = TRUE;
							}
						}

						ULONG DigitsNum = 1;

						ULONG Temp = Num;
						while (Temp > (Base - 1)) {
							Temp /= Base;
							DigitsNum += 1;
						}

						if (ConvWidth > DigitsNum) {
							DigitsNum = ConvWidth;
						}


						if (IsNeg) {
							if (Write == TRUE) Result[ResultIndex] = '-';
							++ResultIndex;
							--DigitsNum;
						}

						CHAR* ResultAdr = NULL;
						if (Write == TRUE) ResultAdr = &(Result[ResultIndex]);

						Temp = Num;
						ResultIndex += DigitsNum;

						if (Write == TRUE) {
							while (DigitsNum > 0) {
								ResultAdr[DigitsNum - 1] = HexChars[Temp % Base];
								Temp /= Base;
								DigitsNum -= 1;
							}
						}
					}
				}
			}
		}
		va_end(VaList);

		if (Write == TRUE) {
			WriteConsoleA(GetStdHandle(STD_OUTPUT_HANDLE), Result, ResultIndex, 0, 0);
			HeapFree(ProcHeap, HEAP_NO_SERIALIZE, Result);
			return 0;
		}

		Result = HeapAlloc(ProcHeap, HEAP_NO_SERIALIZE, (SIZE_T)ResultIndex);
		if (Result == 0) {
			return 2;
		}
		Write = TRUE;
		ResultIndex = 0;

	}

	return 0;
}
