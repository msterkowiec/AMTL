//
// Assertive MultiThreading Library
//
//  Copyright Marcin Sterkowiec, Piotr Tracz, 2021-2022. Use, modification and
//  distribution is subject to license (see accompanying file license.txt)
//

#pragma once

// 
// __AMTL_ASSERTS_ARE_ON__
//
// Main configurable setting of Assertive MultiThreading Library
//
// *** IMPORTANT *** 
//   -> Uncomment this line (#define __AMTL_ASSERTS_ARE_ON__) in order to use features of AMTL (runtime checks)
//   -> Leave it commented out for builds without additional checks (ifndef added to make this setting overridable before inclusion, e.g. for tests)
//
// ====================================
#ifndef __AMTL_ASSERTS_ARE_ON__
// #define __AMTL_ASSERTS_ARE_ON__
#endif
// ====================================

#if defined(__AMTL_ASSERTS_ARE_ON__)

// 
// __AMT_FORCE_SAME_SIZE_FOR_TRIVIAL_TYPES__ 
// (previously called __AMT_USE_HASH_MAP__ but effect seems more important than what is underneath)
//
// Recommended setting for start: on (1). It makes trivial types (amt::int8_t etc.) to have the same sizeof os their equivalents from std and, thus, avoid possible problems with persistence, memcpy etc.
// However it incurs some slowdown due to increased complexity and additional memory usage. If only possible, it might be advisable to make some effort and let code work with this setting off(0) 
// even though it causes e.g. sizeof(amt::int8_t) == 3. Additional drawback of hash_map that the library can no longer be used as header only with it (unless the project consists of single module)
#define __AMT_FORCE_SAME_SIZE_FOR_TRIVIAL_TYPES__ 1

// This setting lets decide if cases of concurrent partial write and partial read (e.g. reading an element when push_back without reallocation takes place) should be reported as an assetion failure
// Recommended setting for starters is on (1). In case it proves to produce false positive (possible in case of thread synchronization targetted for maximum performance), it can be switched off
#define __AMT_REPORT_DOUBTFUL_CASES_WITH_VECTOR__ 1

// This option (default 0) can be on (1) in programs that use trivial/pod data in non standard way. For example let's take struct TrivialMembersOnly { MyInt i; MyFloat f;} and assume it's used in this way:
// TrivialMembersOnly* pMyData = malloc(sizeof(TrivialMembersOnly) * ELEMENTES_NEEDED). There's no problem with it as soon as MyInt and MyFloat are really some int / float types.
// However when developer wants to use it with AMTL and changes definitions e.g. typedef amt::int32_t MyInt, they are no longer trivial/pod types but full fledged objects that need to be constructed but they are not.
// By default, with __AMT_IGNORE_UNREGISTERED_SCALARS__ 0, AMTL will raise assertion failure on first attempt of access to amt::int32_t MyInt that was not constructed properly.
// However when developer wants to do nothing about it and still be able to run test, he/she can just set this option to 1. It should have no side effects.
# define __AMT_IGNORE_UNREGISTERED_SCALARS__ 1

//
// __AMT_TRY_TO_AUTOMATICALLY_WRAP_UP_CONTAINERS_TYPES__
// Strongly recommended setting: off (0)
// 
// This legacy setting can be on(1) or off(0) but 0(off) is strongly recommended.
// Automatic wrapping was tempting at a time but proved to cause many issues - e.g. with ambiguous casts.
// Here is the original description/rationale:
//	// Setting it on will make containers automatically apply a wrapper on scalar types: for example amt::map<int,int> will underneath have std::map<int,amt::int32_t> (or, more prcisely, AMTScalarType<int>)
//	// Such change may make some compilation and/or run-time issues, particularly if persistance, memmove/memcpy/memset involved or interpretation of the area as contiguous is used.
//	// This last case is particularly difficult to detect when interpretation is for reading only (otherwise destructor can detect it by overwritten bytes of counters) :
//	// Example: amt::vector<HANDLE> vecThreadHandles.... WaitForMultipleObjects(nThreads, &vecThreadHandles[0], TRUE, INFINITE);
//	// Such case would reveal in a very nasty and hard to detect manner. That's why it is not recommended to use __AMT_TRY_TO_AUTOMATICALLY_WRAP_UP_CONTAINERS_TYPES__ 1 in the first step,
//	// particularly if application is large and ALL vectors in application were moved from std to amt
// The case mentioned above will not happen with __AMT_FORCE_SAME_SIZE_FOR_TRIVIAL_TYPES__ == 1

#define __AMT_TRY_TO_AUTOMATICALLY_WRAP_UP_CONTAINERS_TYPES__ 0

// ------------------------------------------------------
// Define which functionalities to use:
#ifndef  __AMT_CHECK_MULTITHREADED_ISSUES__
#define __AMT_CHECK_MULTITHREADED_ISSUES__ 1
#endif
#ifndef __AMT_CHECK_ITERATORS_VALIDITY__
#define __AMT_CHECK_ITERATORS_VALIDITY__ 1
#endif
#ifndef __AMT_CHECK_SYNC_OF_ACCESS_TO_ITERATORS__
#define __AMT_CHECK_SYNC_OF_ACCESS_TO_ITERATORS__ 1
#endif
#ifndef __AMT_CHECK_NUMERIC_OVERFLOW__
#define __AMT_CHECK_NUMERIC_OVERFLOW__ 1
#endif
// ------------------------------------------------------

// Use this setting to speed up compilation time (forcing inline may cause huge slowdowns)
#define __AMT_DONT_FORCE_INLINE__ 0

// This setting generally should be off. It is only suitable during unit testing to SetThrowCustomAssertHandler()
#ifndef __AMT_LET_DESTRUCTORS_THROW__
#define __AMT_LET_DESTRUCTORS_THROW__ 0
#endif

// Switch it on only for internal debugging of AMT (e.g. internal assertions will be on)
#ifndef __AMT_DEBUG__
#define __AMT_DEBUG__ 0
#endif

// AMTL asserts in its own way, but sometimes (e.g. during tests) it's better if it is a standard C++ asserion
#ifndef __AMTL_USE_STANDARD_ASSERT__
#define __AMTL_USE_STANDARD_ASSERT__ 0
#endif

#endif
