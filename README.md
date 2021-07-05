//
// Assertive MultiThreading Library
//
//  Copyright Marcin Sterkowiec, Piotr Tracz, 2021. Use, modification and
//  distribution is subject to license (see accompanying file license.txt)
//

[![CMake](https://github.com/msterkowiec/AMTL/actions/workflows/cmake.yml/badge.svg?branch=main)](https://github.com/msterkowiec/AMTL/actions/workflows/cmake.yml)
[![CMake-MSVC](https://github.com/msterkowiec/AMTL/actions/workflows/cmake-msvc.yml/badge.svg)](https://github.com/msterkowiec/AMTL/actions/workflows/cmake-msvc.yml)


# Purpose

Assertive MultiThreading Library (AMT) is intended to have no impact on actual release builds.
Its main purpose is to provide types for special builds (debug or special release with asserts) that can detect errors in code.

AMT provides equivalents of standard types (like std::map, std::vector, std::int8_t etc.) within amt namespace.
In release build these types are typedef'ed back to std, so effectively there is no change in release mode/

# Features

Cases of improper usages that currently can be detected:
* missing thread synchronization (concurrent access to container or a variable that can cause problems),
* numeric overflow
* operations on invalidated iterators

# Usage

All user of AMTL should do to use it is: 
* change in the application under tests the types from namespace std to amt for as many variables/objects as possible (e.g. even replace all std::map, std::vector, std::intXX_t, std::uintXX_t to amt::)
* take a look at amt_config.h and edit it, if needed - there's a macro __AMT_RELEASE_WITH_ASSERTS__: 
  - __AMT_RELEASE_WITH_ASSERTS__ should be off (0) in release builds (in such case AMTL has no effect),
  - __AMT_RELEASE_WITH_ASSERTS__ should be on (1) for test release builds with asserts - here's where features of AMTL can help detect issues

Thus, when using AMTL it is recommended to use the following four various configurations in various stages of development and testing:
- debug,
- release with asserts without optimizations - good for debugging issues
- release with asserts with optimizations - good for regression test suits, slightly worse for debugging due to many symbols optimized out and many calls inlined
- actual release (without asserts and with optimizations)

---------------------------------------------------

Currently AMTL may be considered a working POC.

# What is done:
- amt::map, amt::set
- amt::vector - without iterators and some methods missing (emplace, reserve)
- initial version of control of multithreaded access to trivial types (AMTScalarType)
- numeric overflow (partially)

# To-do, at least:
- customize cassert; e.g. be able to pass ptr to cassert function or force throw instead of displaying assert msg (may be suitable for tests)
- amt::string
- move constructor and move assignment handled in amt::map and amt::vector
- iterators of vector; also missing reserve, emplace
- completion of code for verification of numeric overflow: a) 64-bit types, b) double/float c) double/float on the right of operators
- list, deque, queue, array, forward_list, stack, priority_queue, multiset, multimap
- unordered_map, unordered_set, unordered_multimap, unordered_multiset
- vector<bool>
- make sure that all the traits of types from amt namespace are exactly the same as the traits of their equivalents from std namespace
- polish AMTScalarType and AMTPointerType
- last but not least: a) prepare test suite b) test AMTL with as large codebase as possible - replacing as many types from std with amt as possible. The objective is that all the existing code (at least C++11) should compile successfully with AMTL types without having to add a single type cast in it.

# Known issues
AMTL is a fresh project (its idea sprang in the middle of April 2021), so many things are still missing and many may have been unnoticed, anyway here is the list of possible issues spotted so far:
- it may be not enough to replace amt types with amt "assertive" types to compile successfully - adding manually some casts may be inevitable - e.g. when implicit cast that changes signedness is used in the existing code (anyway such problems with compilation may indicate points in code that require attention - and as such it may be treated as additional value of AMTL not an issue : )
  This is due to the fact that C++ doesn't allow to have more than one suitable operator for conversion, no possibility to have "last_resort" operator, though it seems it would be nice to have it for such wrapper classes like in AMTL
- compilation times in release (with optimizations) sometimes tends to be... almost infinite with VS :) Replacing some __AMT_FORCEINLINE__ with just "inline" may solve the issue (see macro __AMT_DONT_FORCE_INLINE__ in amt_config.h)
- AMTL may raise false alarms/false positives; particularly annoying is the following case with subtraction of two unsigned numeric types:
  amt::uint8_t a = 1;
  amt::uint8_t b = 2;
  amt::int8_t c = a - b; // Here AMTL raises assertion failure, because a - b gives overflow on the type of left operand (which is unsigned), although this is 100% safe with C++ numeric types
  Probably better than abandon usage of AMTL is to explicitly express intention, e.g. in the following way:
  amt::int8_t c = ((std::int8_t)a) - b;
  or better something like this, though for sure it would be cumbersome as hell:
  amt::int8_t c = ((std::make_signed<decltype(a)>::type)a) - b;
  That's why the following macros are available: AMT_CAST_TO_SIGNED(x) and AMT_CAST_TO_UNSIGNED(x) - what's important, they are currently defaulted just to "x" if AMTL feature is off (if amt_config.h leaves __AMT_RELEASE_WITH_ASSERTS__ undefined)

# Origin

It may be worth adding a few words about project origin. As said, its idea sprang in the middle of April 2021 - during bug fixing of a piece of software that solves chess moremovers (it's been developed "after hours" for quite many years and treated as very well tested - large test suite, "robust", "reliable" etc. etc.)
However this time I was a bit lazy and didn't want to put too much effort into finding out the cause of some thread synchronization issue. AMTL, or more exactly some initial class, showed the exact two places in code and exact two threads that clashed.
I must admit I was impressed by the result - practically with no effort I had it solved (AMTL, like a good teacher, showed me: "here and here you have a sync issue"). There was no alternative but to try to continue this...
BTW: since then, AMTL showed me about 10 other issues (about half of them multithreading/sync issues, another half was integer overflow that had been unnoticed earlier - somehow all the tests had been passing...)

# Hopes

Like in the song by Pink Floyd, the hopes are quite high : )
First of all, AMTL seems to have a potential to make C++ software much better tested and much more robust - and, thus, save time, money and maybe even lives (depending on kind of software).
Eventually a similar solution might/should become a part of standard library - and become switchable by some macro - without need to use symbols from amt namespace any more.
