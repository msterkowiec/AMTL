//
// Assertive MultiThreading Library
//
//  Copyright Marcin Sterkowiec, Piotr Tracz, 2021. Use, modification and
//  distribution is subject to license (see accompanying file license.txt)
//

#pragma once


// noexcept
#if defined(_MSC_VER) && __cplusplus < 201402L
#define __NOEXCEPT__
#else
#define __NOEXCEPT__ noexcept
#endif

// __forceinline
#ifdef _MSC_VER
#define __FORCEINLINE__ __forceinline
#elif defined(__GNUC__)
#define __FORCEINLINE__ inline __attribute__((__always_inline__))
#elif defined(__CLANG__)
#if __has_attribute(__always_inline__)
#define __FORCEINLINE__ inline __attribute__((__always_inline__))
#else
#define __FORCEINLINE__ inline
#endif
#else
#define __FORCEINLINE__ inline
#endif

// __declspec(dllexport)
#ifdef _MSC_VER
#define __DLLEXPORT__ __declspec(dllexport)
#else
#define __DLLEXPORT__ 
#endif
