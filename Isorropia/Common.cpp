#include "Config.h"
#include "Common.h"

void DebugPrint(const char* str) {
#ifdef ISORROPIA_DEBUG
	static HANDLE handle = 0;
	if (handle == 0) {
		AllocConsole();
		handle = GetStdHandle(STD_OUTPUT_HANDLE);
	}

	DWORD dwCharsWritten;
	WriteConsoleA(handle, str, (DWORD)strlen(str), &dwCharsWritten, NULL);
#endif
}

void DebugPrintLn(const char* str) {
	DebugPrint(str);
	DebugPrint("\n");
}

void DebugPrintf(const char* fmt, ...) {
#ifdef ISORROPIA_DEBUG
	const int BUF_LEN = 256;
	char buf[BUF_LEN];
	va_list args;
	va_start(args, fmt);
	vsprintf_s(buf, BUF_LEN, fmt, args);
	va_end(args);
	DebugPrint(buf);
#endif
}