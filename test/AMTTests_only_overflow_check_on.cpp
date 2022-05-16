

// Override these macros for sake of tests before inclusion of AMTL headers:
#define __AMTL_ASSERTS_ARE_ON__ 
#define __AMT_CHECK_MULTITHREADED_ISSUES__ 0 // main feature off
#define __AMT_CHECK_ITERATORS_VALIDITY__ 0
#define __AMT_CHECK_SYNC_OF_ACCESS_TO_ITERATORS__ 0
#define __AMT_CHECK_NUMERIC_OVERFLOW__ 1 // the only feature that is on in this test

// These additional settings should be overridden only for sake of tests:
#define __AMT_LET_DESTRUCTORS_THROW__ 1
#define __AMT_DEBUG__ 1

#define __AMT_TEST__ AMTLTest_OverflowOnly

#include "AMTTests_core.cpp"