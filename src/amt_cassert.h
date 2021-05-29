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
#define AMT_CASSERT(a) ((a) ? (void) 1 : __custom_assert<1>(a, __FILE__, __LINE__, _CRT_STRINGIZE(#a)))
#else
	#ifdef _DEBUG
	#define AMT_CASSERT(a) ((a) ? (void) 1 : assert(a))
	#else
	#define AMT_CASSERT(a) ((a) ? (void) 1 : __custom_assert<1>(a, __FILE__, __LINE__))
	#endif
#endif

	#ifdef _WIN32
	#include <string>
	#include <cstdio>
	#include <stdio.h>
	#include <wtypes.h>
	#include <processthreadsapi.h>
	#include <windows.h>

	template<bool>
	inline void __custom_assert(bool a, const char* szFileName, long lLine, const char* szDesc)
	{		
		if (!a)
		{
			static const size_t BUFLEN = 1024;
			char msg[BUFLEN];
			DWORD nCurrentThreadId = GetCurrentThreadId();
			_snprintf(msg, BUFLEN - 1,  "Assertion failed at line %d in thread id %d in file %s\n%s", lLine, nCurrentThreadId, szFileName, szDesc);
			msg[BUFLEN - 1] = 0;

			MessageBoxA(NULL, msg, (LPCSTR)"Assertion failed", MB_OK | MB_ICONEXCLAMATION);
		}
	}
	#else
	#include <iostream>
	#include <thread>
	#include <cstdio>
	template<bool>
	inline void __custom_assert(bool a, const char* szFileName, long lLine)
	{
		std::cout << "Assertion failure in file " << szFileName << " at line " << lLine << ". Thread id = " <<  std::this_thread::get_id() << ". Press <ENTER> to continue.\n";
		getchar();
	}
	#endif

#else
#define AMT_CASSERT(a)
#endif
