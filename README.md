//
// Assertive MultiThreading Library
//
//  Copyright Marcin Sterkowiec, Piotr Tracz, 2021-2022. Use, modification and
//  distribution is subject to license (see accompanying file license.txt)
//

[![CMake-MSVC-2019](https://github.com/msterkowiec/AMTL/actions/workflows/cmake-msvc-2019.yml/badge.svg)](https://github.com/msterkowiec/AMTL/actions/workflows/cmake-msvc-2019.yml)
[![CMake-MSVC-2022](https://github.com/msterkowiec/AMTL/actions/workflows/cmake-msvc-2022.yml/badge.svg)](https://github.com/msterkowiec/AMTL/actions/workflows/cmake-msvc-2022.yml)
[![CMake-GCC](https://github.com/msterkowiec/AMTL/actions/workflows/cmake-gcc.yml/badge.svg?branch=main)](https://github.com/msterkowiec/AMTL/actions/workflows/cmake-gcc.yml)
[![CMake-CLang](https://github.com/msterkowiec/AMTL/actions/workflows/cmake-clang.yml/badge.svg?branch=main)](https://github.com/msterkowiec/AMTL/actions/workflows/cmake-clang.yml)


# Purpose

Assertive MultiThreading Library (AMT) is intended to have no impact on actual release builds.
Its main purpose is to provide types for special builds (debug or special release with asserts) that can detect errors in code.

AMT provides equivalents of standard types (like std::map, std::vector, std::int8_t etc.) within amt namespace.
In release build these types are typedef'ed back to std, so effectively there is no change in release mode.

# Features

Cases of improper usages that currently can be detected:
* missing thread synchronization (concurrent access to container or a variable that can cause problems),
* numeric overflow
* operations on invalidated iterators
* operations on uninitialized memory (in one of the two main modes of the library; it is actually not exactly a feature but a side effect - but a very valuable one - more about it below)

# Usage

All a user of AMTL should do to use it is:
* change in the application under tests the types from namespace std to amt for as many variables/objects as possible (e.g. even replace all std::map, std::vector, std::intXX_t, std::uintXX_t to amt::)
* take a look at amt_config.h and edit it, if needed - there's a macro ____AMTL_ASSERTS_ARE_ON____: 
  - ____AMTL_ASSERTS_ARE_ON____ should be off (0) in "real" release builds (in such case AMTL has no effect, i.e. amt:: types resolve to std:: types),
  - ____AMTL_ASSERTS_ARE_ON____ should be on (1) for test builds with asserts - here's where features of AMTL can help detect issues (release builds with asserts are quite good since they allow to overcome slowness of asserts)

Thus, when using AMTL it is recommended to use the following four various configurations in various stages of development and testing:
- debug,
- release with asserts without optimizations - good for debugging issues
- release with asserts with optimizations - good for regression test suits, slightly worse for debugging due to many symbols optimized out and many calls inlined
- actual release (without asserts and with optimizations)

# Two main modes of the library
AMTL works in two main modes:
* let's call it __easy but very slow__ mode - with ____AMT_FORCE_SAME_SIZE_FOR_TRIVIAL_TYPES____ == 1 
* let's call it __more difficult but fast and better__ mode - with ____AMT_FORCE_SAME_SIZE_FOR_TRIVIAL_TYPES____ == 0

To be concise, I'll call these modes __easy__ and __extended__. In the extended mode there's a minor complication: all the trivial types (like amt::uint8_t or amit::int32_t) have two additional bytes in its size - these bytes may be called "current thread reference counts" (for read and write operations, respectively). In the easy mode, these two bytes are stored in the external global singleton hash_map. No need to mention how costly every operation on such integer is, if any operation on it involves lookup in such an external structure. Extended mode is not that slow any more but a developer has to be slightly more cautious, because some memcpy'ing or memmoving that exists in code, may start to work not exactly as it was intended (particularly if serialization on disk is involved). That's why in the first step the easy mode is recommended. When everything starts to work fine in easy mode, it might be then good to try to switch application with AMTL to extended mode. The build with AMTL will not only start to run much much faster but - as a sort of a bonus - it will also be able to detect another nasty type of flaws in application - operations on uninitialized memory.

# C++ version
Tested with C++11 and C++17
AMTL is meant to be applicable to existing code to create special builds with asserts - that's why it is written to be compilable with older versions of C++ (but at least C++11).
Yet older versions (C++98/03) are not supported and, as for now, there is no plan to include such support.
Not tested yet with C++20/23 (probably quite much effort needed to make AMTL suitable for C++20 code)

---------------------------------------------------

Currently AMTL may be considered a working POC.

# What is done:
- amt::map, amt::set
- amt::vector - without iterators
- verification of multithreaded access to trivial types (AMTScalarType) e.g. amt::int8_t
- numeric overflow (partially)

# To-do, at least:
- amt::bool + check floating point types
- customize cassert; e.g. be able to pass ptr to cassert function or force throw instead of displaying assert msg (may be suitable for tests)
- amt::string
- iterators of vector
- completion of code for verification of numeric overflow: a) 64-bit types, b) double/float c) double/float on the right of operators
- list, deque, queue, array, forward_list, stack, priority_queue, multiset, multimap
- unordered_map, unordered_set, unordered_multimap, unordered_multiset
- vector < bool >
- make sure that all the traits of types from amt namespace are exactly the same as the traits of their equivalents from std namespace
- polish AMTScalarType and AMTPointerType
- last but not least: a) prepare test suite and b) examples c) test AMTL with as large codebase as possible - replacing as many types from std with amt as possible. The objective is that all the existing code (at least C++11) should compile successfully with AMTL types without having to add a single type cast in it.

# Known issues
AMTL is a fresh project (its idea sprang in the middle of April 2021), so many things are still missing and many may have been unnoticed, anyway here is the list of possible issues spotted so far:
- compilation times in release (with optimizations) sometimes tends to be... almost infinite with VS :) Replacing some __AMT_FORCEINLINE__ with just "inline" may solve the issue (see macro __AMT_DONT_FORCE_INLINE__ in amt_config.h)
- some explicit casts may be needed particularly in case of conditional operator ? (if one of the options is an AMTL scalar, e.g. amt::int32t, and the other is not wrapped up, e.g. just an int); an AMTL wrapper cannot be used in a bit field either,
- if some piece of code already contains some implicit casts, adding another level of complexity (AMTL wrappers) may break build with AMTL (C++ will not accept two implicit casts); in such case the code should be simplified in some way (BTW: once, with MSVC2019, I even came accross "Internal compiler error" with explanation to try to simplify the code around the place when this error occurred - anyway it shows that AMTL wrappers may sometimes introduce additional level of complexity that may even be too much for compilers - and that simply writing code without too many implicit casts may make it easier and let avoid reading strange compilation error messages : )

As a consequence here is a proposition of changes in C++ to consider that might make things a bit easier:
  
# Pottential changes in C++ language that might make using wrappers easier
It might be worth adding a new keyword (or a context specific token) "wraps", for example:
```
      template<typename T>
      class/struct X wraps T 
```	  
Requirement: the class/struct X that wraps type T has to have operator T defined (otherwise compilation error should be raised).
Using keyword/token "wraps" would have the following effect:
- in case of a conditional operator (?), which is very strict as far as types are concerned (both types have to be the same), usage of a wrapper type (X) should be treated as equivalent of usage of the wrapped type (T)
- an instance of the wrapper type (X) can be used in a context that requires an implicit cast from type T (in other words: the cast from X to T should not be treated as an implicit cast but both types X and T should be treated as equivalent, whenever an implicit cast from T would be applied by compiler but usage of type X would be illegal)
- if class/struct X (wrapper) is used in a bit field and the wrapped type (T) is a fundamental type, the fundamental type has precedence (wrapper is skipped)
	
# Origin

It may be worth adding a few words about project origin. As said, its idea sprang in the middle of April 2021 - during bug fixing of a piece of software that solves chess moremovers (it's been developed "after hours" for quite many years and treated as very well tested - large test suite, "robust", "reliable" etc. etc.)
However this time I was a bit lazy and didn't want to put too much effort into finding out the cause of some thread synchronization issue. AMTL, or more exactly some initial class, showed the exact two places in code and exact two threads that clashed.
I must admit I was impressed by the result - practically with no effort I had it solved (AMTL, like a good teacher, showed me: "here and here you have a sync issue"). There was no alternative but to try to continue this...
BTW: since then, AMTL showed me about 10 other issues (about half of them multithreading/sync issues, another half was integer overflow that had been unnoticed earlier - somehow all the tests had been passing...)

# Detection of operations on uninitialized memory ("side effect" feature)

It works in extended mode (with __AMT_FORCE_SAME_SIZE_FOR_TRIVIAL_TYPES__ == 0) because "thread reference counters" are located within size of the object, so they will be within the uninitialized memory, so AMTL assertion will be very likely - particularly if the memory is marked with debug magic values. Without AMTL such uninitialized access is almost sure to go unnoticed both in debug and release builds, like in the following simplified case (based on a real life code - production code - in which such issue was detected by AMTL)

```
#include <vector>
#include <numeric>
#include <algorithm>
#include <iostream>

class Data
{
	double* pData_;

public:
	Data(double* pData) : pData_(pData)
	{

	}
	double Product(const std::vector<double>& o)
	{
		return std::inner_product(o.begin(), o.end(), pData_, 0.0);
	}

};

int main()
{
	static const size_t SIZE = 4;
	double* pData = new double[SIZE];
	std::vector<double> vec(SIZE+1); // let's assume there is "+1" added by mistake - the problem doesn't reveal in the debug or release, 
	                                 // unless surprisigly "in production" (if pData is accidentally allocated on the edge of a memory page, 
	                                 // causing access violation exception). The result (dot product) is always ok (20), since the uninitialized value after the buffer
	                                 // is always multiplied by zero (at the end of the vec).
	for (size_t i = 0; i < SIZE; ++i)
	{
		vec[i] = i + 1;
		pData[i] = 4 - i;
	}
	Data data(pData);
	double ret = data.Product(vec);

	// Let's test the correctness of the result:
	if (ret == 20)	       		 		  //                               1 * 4 + 2 * 3 + 3 * 2 + 4 * 1 == 20, OK!
		std::cout << "OK - the result is 20!!\n"; // but we really are calculating 1 * 4 + 2 * 3 + 3 * 2 + 4 * 1 + 0 * something_uninitialized_after_the_buffer
	else			                          //                                                             ==============================================
					              	  // so we may get a random crash... the issue doesn't reveal neither in debug nor release mode...
		std::cout << "Something went wrong - the result is: " << ret << "\n";
}
```
										
Usage of amt::_double instead of double lets detect the problem - of course AMTL doesn't say in its assertion failure "the memory must be uninitialized" - but seeing "thread reference counters" have values 205 and 206 (BTW: 205 == 0xCD == MSVC debug filler for uninitialized heap) makes it obvious enough.

To make the case more complete: just add the following two lines in the beginning to be able to use AMTL:
```
#define __AMTL_ASSERTS_ARE_ON__ // comment it out in order to have a normal build without AMTL
#include "amt_pod.h" // make sure there's #define __AMT_FORCE_SAME_SIZE_FOR_TRIVIAL_TYPES__ 0 in amt_config.h (this feature/side-effect works only in "extended" mode of AMTL)
// TODO: replace all usages of "double"	with amt::_double
```
		
# Hopes

Like in the song by Pink Floyd, the hopes are quite high : )
First of all, AMTL seems to have a potential to make C++ software much better tested and much more robust - and, thus, save time, money and maybe even lives (depending on kind of software).
Eventually a similar solution might/should become a part of standard library - and become switchable by some macro - without need to use symbols from amt namespace any more.
