#ifndef __COMMON_H__
#define __COMMON_H__

#ifndef WINVER
#define WINVER 0x0601
#endif

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601  
#endif

#include <Windows.h>
#pragma comment(lib,"winmm.lib")

#include <dxgi.h>
#include <dxgi1_2.h>
#include <d3d11.h>

#include <Psapi.h>

#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include <math.h>
#include <cstdarg>
#include <cassert>
#include <cstdint>

#define ERR_NONE	0
#define ERR_FAILURE 1
typedef __int32 err_t;

// There are 10e5 microseconds in a second
#define _10E5 1000000
typedef __int64 microsecond_t;
typedef __int64 millisecond_t;

void DebugPrint(const char* str);
void DebugPrintLn(const char* str);
void DebugPrintf(const char* str, ...);

#ifndef LOG
#define LOG(X) DebugPrintLn(X)
#endif

#ifndef TRACE
#define TRACE(X)
//#define TRACE(X) DebugPrintLn(X)
#endif

#endif // __COMMON_H__
