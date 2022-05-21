

// Override these macros for sake of tests before inclusion of AMTL headers:
// #define __AMTL_ASSERTS_ARE_ON__ // the main macro of AMTL is now not defined (line commented out) - so we want to make sure, that all the code works fine without AMTL but with amt namespace and its typedefs
#define __AMT_CHECK_MULTITHREADED_ISSUES__ 1
#define __AMT_CHECK_ITERATORS_VALIDITY__ 1
#define __AMT_CHECK_SYNC_OF_ACCESS_TO_ITERATORS__ 1
#define __AMT_CHECK_NUMERIC_OVERFLOW__ 1

// These additional settings should be overridden only for sake of tests:
#define __AMT_LET_DESTRUCTORS_THROW__ 1
#define __AMT_DEBUG__ 1

#define __AMT_TEST__ AMTLTest_AllFeaturesOFF

#include "AMTTests_core.cpp"