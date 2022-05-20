//
// Assertive MultiThreading Library
//
//  Copyright Marcin Sterkowiec, Piotr Tracz, 2021-2022. Use, modification and
//  distribution is subject to license (see accompanying file license.txt)
//

[![CMake-MSVC](https://github.com/msterkowiec/AMTL/actions/workflows/cmake-msvc.yml/badge.svg)](https://github.com/msterkowiec/AMTL/actions/workflows/cmake-msvc.yml)
[![CMake-GCC](https://github.com/msterkowiec/AMTL/actions/workflows/cmake-gcc.yml/badge.svg?branch=main)](https://github.com/msterkowiec/AMTL/actions/workflows/cmake-gcc.yml)


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

# Usage

All a user of AMTL should do to use it is:
* change in the application under tests the types from namespace std to amt for as many variables/objects as possible (e.g. even replace all std::map, std::vector, std::intXX_t, std::uintXX_t to amt::)
* take a look at amt_config.h and edit it, if needed - there's a macro __AMTL_ASSERTS_ARE_ON__: 
  - __AMTL_ASSERTS_ARE_ON__ should be off (0) in "real" release builds (in such case AMTL has no effect, i.e. amt:: types resolve to std:: types),
  - __AMTL_ASSERTS_ARE_ON__ should be on (1) for test builds with asserts - here's where features of AMTL can help detect issues (release builds with asserts are quite good since they allow to overcome slowness of asserts)

Thus, when using AMTL it is recommended to use the following four various configurations in various stages of development and testing:
- debug,
- release with asserts without optimizations - good for debugging issues
- release with asserts with optimizations - good for regression test suits, slightly worse for debugging due to many symbols optimized out and many calls inlined
- actual release (without asserts and with optimizations)

# C++ version
Tested with C++11 and C++17
AMTL is meant to be applicable to existing code to create special builds with asserts - that's why it is written to be compilable with older versions of C++ (but at least C++11).
Yet older versions (C++98/03) are not supported and, as for now, there is no plan to include such support.
Not tested yet with C++20/23 (probably quite much effort needed to make AMTL suitable for C++20 code)

---------------------------------------------------

Currently AMTL may be considered a working POC.

# What is done:
- amt::map, amt::set
- amt::vector - without iterators and some methods missing (emplace, reserve)
- verification of multithreaded access to trivial types (AMTScalarType) e.g. amt::int8_t
- numeric overflow (partially)

# To-do, at least:
- amt::bool + check floating point types
- versions of containers' methods with hint
- customize cassert; e.g. be able to pass ptr to cassert function or force throw instead of displaying assert msg (may be suitable for tests)
- amt::string
- iterators of vector; also missing reserve, emplace
- completion of code for verification of numeric overflow: a) 64-bit types, b) double/float c) double/float on the right of operators
- list, deque, queue, array, forward_list, stack, priority_queue, multiset, multimap
- unordered_map, unordered_set, unordered_multimap, unordered_multiset
- vector<bool>
- make sure that all the traits of types from amt namespace are exactly the same as the traits of their equivalents from std namespace
- polish AMTScalarType and AMTPointerType
- last but not least: a) prepare test suite and b) examples c) test AMTL with as large codebase as possible - replacing as many types from std with amt as possible. The objective is that all the existing code (at least C++11) should compile successfully with AMTL types without having to add a single type cast in it.

# Known issues
AMTL is a fresh project (its idea sprang in the middle of April 2021), so many things are still missing and many may have been unnoticed, anyway here is the list of possible issues spotted so far:
- compilation times in release (with optimizations) sometimes tends to be... almost infinite with VS :) Replacing some __AMT_FORCEINLINE__ with just "inline" may solve the issue (see macro __AMT_DONT_FORCE_INLINE__ in amt_config.h)
- some explicit casts may be needed particularly in case of conditional operator ? (if one of the options is an AMTL scalar, e.g. amt::int32t, and the other is not wrapped up, e.g. just an int)
- if some piece of code already contains some implicit casts, adding another level of complexity (AMTL wrappers) may break build with AMTL (C++ will not accept two implicit casts); in such case the code should be simplified in some way (BTW: once, with MSVC2019, I even came accross "Internal compiler error" with explanation to try to simplify the code around the place when this error occurred - anyway it shows that AMTL wrappers may sometimes introduce additional level of complexity that may even be too much for compilers - and that simply writing code without too many implicit casts may make it easier and let avoid reading strange compilation error messages : )
  
# Origin

It may be worth adding a few words about project origin. As said, its idea sprang in the middle of April 2021 - during bug fixing of a piece of software that solves chess moremovers (it's been developed "after hours" for quite many years and treated as very well tested - large test suite, "robust", "reliable" etc. etc.)
However this time I was a bit lazy and didn't want to put too much effort into finding out the cause of some thread synchronization issue. AMTL, or more exactly some initial class, showed the exact two places in code and exact two threads that clashed.
I must admit I was impressed by the result - practically with no effort I had it solved (AMTL, like a good teacher, showed me: "here and here you have a sync issue"). There was no alternative but to try to continue this...
BTW: since then, AMTL showed me about 10 other issues (about half of them multithreading/sync issues, another half was integer overflow that had been unnoticed earlier - somehow all the tests had been passing...)

# Hopes

Like in the song by Pink Floyd, the hopes are quite high : )
First of all, AMTL seems to have a potential to make C++ software much better tested and much more robust - and, thus, save time, money and maybe even lives (depending on kind of software).
Eventually a similar solution might/should become a part of standard library - and become switchable by some macro - without need to use symbols from amt namespace any more.
