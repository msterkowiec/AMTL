//
// Assertive MultiThreading Library
//
//  Copyright Marcin Sterkowiec, Piotr Tracz, 2021. Use, modification and
//  distribution is subject to license (see accompanying file license.txt)
//

#pragma once

#include "amt_config.h"

// noexcept
#if defined(_MSC_VER) && __cplusplus < 201402L
#define __AMT_NOEXCEPT__
#else
#define __AMT_NOEXCEPT__ noexcept
#endif

// __forceinline
#ifdef _MSC_VER
	#if __AMT_DONT_FORCE_INLINE__ > 0
	#define __AMT_FORCEINLINE__ inline
	#else
	#define __AMT_FORCEINLINE__ __forceinline
	#endif
#elif defined(__GNUC__)
	#if __AMT_DONT_FORCE_INLINE__ > 0
	#define __AMT_FORCEINLINE__ inline
	#else
	#define __AMT_FORCEINLINE__ inline __attribute__((__always_inline__))
	#endif
#elif defined(__CLANG__)
#if __has_attribute(__always_inline__)
	#if __AMT_DONT_FORCE_INLINE__ > 0
	#define __AMT_FORCEINLINE__ inline
	#else
	#define __AMT_FORCEINLINE__ inline __attribute__((__always_inline__))
	#endif
#else
#define __AMT_FORCEINLINE__ inline
#endif
#else
#define __AMT_FORCEINLINE__ inline
#endif

// __declspec(dllexport)
#ifdef _MSC_VER
#define __DLLEXPORT__ __declspec(dllexport)
#else
#define __DLLEXPORT__ 
#endif
