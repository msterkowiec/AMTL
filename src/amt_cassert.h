//
// Assertive MultiThreading Library
//
//  Copyright Marcin Sterkowiec, Piotr Tracz, 2021. Use, modification and
//  distribution is subject to license (see accompanying file license.txt)
//

#pragma once

#include "amt_config.h" 

#include <assert.h>

#if defined(_DEBUG) || defined (__AMT_RELEASE_WITH_ASSERTS__)

#include <set>
#ifdef _WIN32
#define AMT_CASSERT(a) ((a) ? 1 : __custom_assert<1>(a, __FILE__, __LINE__, _CRT_WIDE(#a)))
#endif

#include <string>
#include <stdio.h>

	#if defined(_WIN32) 
	#include <wtypes.h>
	#include <processthreadsapi.h>
	#include <windows.h>

	template<bool>
	inline void __custom_assert(bool a, const char* szFileName, long lLine, const WCHAR* wszDesc)
	{
		if (!a)
		{
			char msg[1024];
			DWORD nCurrentThreadId = GetCurrentThreadId();
			sprintf(msg, "Assertion failed at line %d in thread id %d in file %s.", lLine, nCurrentThreadId, szFileName);

			MessageBoxA(NULL, msg, (LPCSTR)"Assertion failed", MB_OK | MB_ICONEXCLAMATION);
		}
	}
	#else
	#define AMT_CASSERT(a)
	#endif

#else
#define AMT_CASSERT(a)
#endif


