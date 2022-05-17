//
// Assertive MultiThreading Library
//
//  Copyright Marcin Sterkowiec, Piotr Tracz, 2021-2022. Use, modification and
//  distribution is subject to license (see accompanying file license.txt)
//

#pragma once

#include "amt_config.h"

// noexcept; note that with MSVC the line below requires compilation with option /Zc:__cplusplus to make value of macro __cplusplus correct
#if defined(_MSC_VER) && _MSVC_LANG < 201402L
#define __AMT_NOEXCEPT__
#define __AMT_NOEXCEPT_FALSE__
#define __AMT_CONSTEXPR__ const
#define __AMT_IF_CONSTEXPR__
#else
#define __AMT_NOEXCEPT__ noexcept
#define __AMT_NOEXCEPT_FALSE__ noexcept(false)
#define __AMT_CONSTEXPR__ constexpr
#if __cplusplus >= 201606L || (defined(_MSC_VER) && _MSVC_LANG >= 201606L)
#define __AMT_IF_CONSTEXPR__ constexpr
#else
#define __AMT_IF_CONSTEXPR__
#endif
#endif

#if __AMT_LET_DESTRUCTORS_THROW__
#define __AMT_CAN_THROW__ __AMT_NOEXCEPT_FALSE__
#else
#define __AMT_CAN_THROW__
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
