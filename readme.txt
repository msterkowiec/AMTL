//
// Assertive MultiThreading Library
//
//  Copyright Marcin Sterkowiec, Piotr Tracz, 2021. Use, modification and
//  distribution is subject to license (see accompanying file license.txt)
//

Assertive MultiThreading Library (AMT) is intended to have no impact on actual release builds.
Its main purpose is to provide types for special builds (debug or special release with asserts) that can detect errors in code.

AMT provides equivalents of standard types (like std::map, std::vector, std::int8_t etc.) within amt namespace.
In release build these types are typedef'ed back to std, so effectively there is no change in release mode/

Cases of improper usages that currently can be detected:
* missing thread synchronization (concurrent access to container or a variable that can cause problems),
* numeric overflow
* operations on invalidated iterators

All user of AMTL should do to use it is: 
* change in the application under tests the types from namespace std to amt for as many variables/objects as possible (e.g. even replace all std::map, std::vector, std::intXX_t, std::uintXX_t to amt::)
* take a look at amt_config.h and edit it, if needed - there's a macro __AMT_RELEASE_WITH_ASSERTS__: 
  - __AMT_RELEASE_WITH_ASSERTS__ should be off (0) in release builds (in such case AMTL has no effect),
  - __AMT_RELEASE_WITH_ASSERTS__ should be on (1) for test release builds with asserts - here's where features of AMTL can help detect issues

Thus, when using AMTL it is recommended to use the following four various configurations in various starges of development and testing:
- debug,
- release with asserts without optimizations - good for debugging issues
- release with asserts with optimizations - good for regression test suits, slightly worse for debugging due to many symbols optimized out and many calls inlined
- actual release (without asserts and with optimizations)

---------------------------------------------------

Currently AMTL may be considered a working POC.

What is done:
- amt::map
- amt::vector - without iterators and some methods missing (emplace, reserve)
- initial version of control of multithreaded access to trivial types (AMTScalarType)
- numeric overflow (partially)

To-do, at least:
- customize cassert; e.g. be able to pass ptr to cassert function or force throw instead of displaying assert msg (may be suitable for tests)
- polish AMTScalarType, fix AMTPointerType (the latter commented out for now)
- amt::string
- move constructor and move assignment handled in amt::map and amt::vector
- iterators of vector; also missing reserve, emplace
- completion of code for verification of numeric overflow: a) 64-bit types, b) double/float c) double/float on the right of operators
- set, list, deque, queue, array, forward_list, stack, priority_queue, multiset, multimap
- unordered_map, unordered_set, unordered_multimap, unordered_multiset
- vector<bool>
- last but not least: a) prepare test suite b) test AMTL with as large codebase as possible (replacing as many types from std with amt as possible)