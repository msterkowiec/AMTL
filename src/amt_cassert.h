//
// Assertive MultiThreading Library
//
//  Copyright Marcin Sterkowiec, Piotr Tracz, 2021-2022. Use, modification and
//  distribution is subject to license (see accompanying file license.txt)
//

#pragma once

#include <exception>
#include "amt_config.h" 
#include "amt_compat.h" 

#include <assert.h>

namespace amt {
	typedef void(*CustomAssertHandlerPtr)(bool, const char*, long, const char*); // the parameters are: isAssertionOk, szFileName, lLine, szDesc

	struct AMTCassertException : std::exception
	{
		std::string sFileName;
		long lLine;
		std::string sDesc;

		AMTCassertException(const char* szFileName, long lLine, const char* szDesc) : sFileName(szFileName), lLine(lLine), sDesc(szDesc)
		{
		}
	};
}

#ifdef __AMTL_ASSERTS_ARE_ON__

#include <set>

#ifdef _WIN32
#define AMT_CASSERT(a) ((a) ? (void) 1 : amt::__custom_assert<1>(a, __FILE__, __LINE__, _CRT_STRINGIZE(#a)))
#else
	#if !defined(NDEBUG) 
	#define AMT_CASSERT(a) ((a) ? (void) 1 : assert(a))
	#else
	#define AMT_CASSERT(a) ((a) ? (void) 1 : amt::__custom_assert<1>(a, __FILE__, __LINE__, #a))
	#endif
#endif

	#ifdef _WIN32
	#include <string>
	#include <cstdio>
	#include <stdio.h>
	#include <wtypes.h>
	#include <processthreadsapi.h>
	#include <windows.h>
	#include <string.h>

	// Default handler for Windows:
	namespace amt{
	template<bool>
	inline void __custom_assert(bool a, const char* szFileName, long lLine, const char* szDesc)
	{		
		if (!a)
		{
			static const size_t BUFLEN = 1024;
			static CustomAssertHandlerPtr s_pCustomAssertHandler = nullptr;
			if (lLine == -1 && strcmp(szFileName, "amt::SetCustomAssertHandler") == 0)
			{
				s_pCustomAssertHandler = (CustomAssertHandlerPtr) szDesc;
				return;
			}
			if (s_pCustomAssertHandler != nullptr)
			{
				(*s_pCustomAssertHandler)(a, szFileName, lLine, szDesc); // call custom handler
				return;
			}

			#if __AMTL_USE_STANDARD_ASSERT__
			assert(a);
			#if !defined(_DEBUG)
			throw std::runtime_error("AMTL assertion failed");
			#endif
			#else
			char msg[BUFLEN];
			DWORD nCurrentThreadId = GetCurrentThreadId();
			_snprintf(msg, BUFLEN - 1,  "Assertion failed at line %d in thread id %d in file %s\n%s", lLine, nCurrentThreadId, szFileName, szDesc);
			msg[BUFLEN - 1] = 0;

			MessageBoxA(NULL, msg, (LPCSTR)"Assertion failed", MB_OK | MB_ICONEXCLAMATION);
			#endif
		}
	}
	} // namespace amt
	
	#else
	#include <iostream>
	#include <thread>
	#include <cstdio>

	namespace amt{
    // Default handler for non-Windows systems:
	template<bool>
	inline void __custom_assert(bool a, const char* szFileName, long lLine, const char* szDesc)
	{
		static CustomAssertHandlerPtr s_pCustomAssertHandler = nullptr;
		if (lLine == -1 && strcmp(szFileName, "amt::SetCustomAssertHandler") == 0)
		{
			s_pCustomAssertHandler = (CustomAssertHandlerPtr) szDesc;
			return;
		}
		if (s_pCustomAssertHandler != nullptr)
		{
			(*s_pCustomAssertHandler)(a, szFileName, lLine, szDesc); // call custom handler
			return;
		}

		#if __AMTL_USE_STANDARD_ASSERT__
		assert(a);
		#if defined(NDEBUG)
		throw std::runtime_error("AMTL assertion failed");
		#endif
		#else
		std::cout << "Assertion failure in file " << szFileName << " at line " << lLine << ". Thread id = " <<  std::this_thread::get_id() << ": " << szDesc << " Press <ENTER> to continue.\n";
		getchar();
		#endif
	}
	} // namespace amt
	#endif

	namespace amt
	{

		template<bool>
		void ThrowCustomAssertHandler(bool a, const char* szFileName, long lLine, const char* szDesc)
		{
			if (!a)
				throw AMTCassertException(szFileName, lLine, szDesc); 
		}

		template<bool>
		void SetCustomAssertHandler(CustomAssertHandlerPtr funcPtr)
		{
			__custom_assert<1>(false, "amt::SetCustomAssertHandler", -1, (const char*)funcPtr); // dirty trick that allows to keep the module stateless (use static variable inside __custom_assert)
		}

		template<bool>
		void SetThrowCustomAssertHandler()
		{
			__custom_assert<1>(false, "amt::SetCustomAssertHandler", -1, (const char*)(CustomAssertHandlerPtr)&ThrowCustomAssertHandler<0>); // dirty trick that allows to keep the module stateless (use static variable inside __custom_assert)
		}

	}

#else
#define AMT_CASSERT(a)

namespace amt
{
	template<bool>
	void SetCustomAssertHandler(CustomAssertHandlerPtr funcPtr) {}
	template<bool>
	void SetThrowCustomAssertHandler() {}	
}
#endif
