//
// NOTE: this file is intended to be included from another AMTTests*.cpp file with specific features on or off
//

// Make sure this line is commented out before committing to github
// #define __AMT_TEST_WITHOUT_GTEST__

#ifndef __AMT_TEST_WITHOUT_GTEST__
#define __AMTL_USE_STANDARD_ASSERT__ 1
#endif

#include <thread>
#include <mutex>
#include <atomic>
#include <cstdlib>
#include <algorithm>
#include <vector>
#include <random>
#include <string>
#include <chrono>
#include <iostream>
#include <time.h> 

// -------------------------------
#if defined(__AMTL_ASSERTS_ARE_ON__) && __AMT_CHECK_MULTITHREADED_ISSUES__
#define AMTL_MAIN_FEATURE_ON 1
#else
#define AMTL_MAIN_FEATURE_ON 0
#endif

#if defined(__AMTL_ASSERTS_ARE_ON__) &&  __AMT_CHECK_ITERATORS_VALIDITY__
#define AMTL_CHECK_ITERATORS_VALIDITY_ON 1
#else
#define AMTL_CHECK_ITERATORS_VALIDITY_ON 0
#endif

#if defined(__AMTL_ASSERTS_ARE_ON__) &&  __AMT_CHECK_SYNC_OF_ACCESS_TO_ITERATORS__
#define AMTL_CHECK_SYNC_OF_ACCESS_TO_ITERATORS_ON 1
#else
#define AMTL_CHECK_SYNC_OF_ACCESS_TO_ITERATORS_ON 0
#endif

#if defined(__AMTL_ASSERTS_ARE_ON__) &&  __AMT_CHECK_NUMERIC_OVERFLOW__
#define AMTL_CHECK_NUMERIC_OVERFLOW_ON 1
#else
#define AMTL_CHECK_NUMERIC_OVERFLOW_ON 0
#endif

#if defined(__AMTL_ASSERTS_ARE_ON__)
#define AMTL_ON 1
#else
#define AMTL_ON 0
#endif

#define AMTL_OPTIONS_HASH (AMTL_ON?(((int)AMTL_MAIN_FEATURE_ON) + (((int)AMTL_MAIN_FEATURE_ON) << 1) + (((int)AMTL_CHECK_ITERATORS_VALIDITY_ON) << 2) + (((int)AMTL_CHECK_SYNC_OF_ACCESS_TO_ITERATORS_ON) << 3) + (((int)AMTL_CHECK_NUMERIC_OVERFLOW_ON) << 4)) : 0)

// ----------------------------------

#ifdef __AMT_TEST_WITHOUT_GTEST__
#define TEST(TestSuiteName, TestName) void TestSuiteName##TestName()
#define RUNTEST(TestSuiteName, TestName) TestSuiteName##TestName()
#if defined(__AMTL_ASSERTS_ARE_ON__)
#define EXPECT_EQ(a,b) AMT_CASSERT((a)==(b))
#define EXPECT_NE(a,b) AMT_CASSERT((a)!=(b))
#define EXPECT_GE(a,b) AMT_CASSERT((a)>=(b))
#else
void cassert(bool b)
{
	if (!b)
		throw std::string("Test failed"); // let it be like this in this case
}
#define EXPECT_EQ(a,b) cassert((a)==(b))
#define EXPECT_NE(a,b) cassert((a)!=(b))
#define EXPECT_GE(a,b) cassert((a)>=(b))
#endif
#else
#include <gtest/gtest.h>
#endif

#include "amt_vector.h"
#include "amt_pod.h"
#include "amt_map.h"
#include "amt_set.h"
#include "amt_string.h"
#include "amt_wstring.h"
#include "amt_rawdatadebugchecker.h"

template<typename U, typename V>
__AMT_CONSTEXPR__ bool AreNumericTypesEquivalent()
{
	typedef typename amt::UnwrappedType<U, amt::is_specialization<U, amt::AMTScalarType>::value>::type UType;
	typedef typename amt::UnwrappedType<V, amt::is_specialization<V, amt::AMTScalarType>::value>::type VType;

	if (sizeof(UType) == sizeof(VType))
		if (std::is_floating_point<UType>::value == std::is_floating_point<VType>::value)
			if (std::is_floating_point<UType>::value)
				return true;
			else
				if (std::is_signed<UType>::value == std::is_signed<VType>::value)
					return true;

	return false;
}

size_t AssertionFailedSilently = false;

void SilentCustomAssertHandler(bool a, const char* szFileName, long lLine, const char* szDesc)
{
	if (!a)
		++AssertionFailedSilently;
}

TEST(__AMT_TEST__, BasicTest){

	std::atomic<int64_t> ati(5);
	amt::int64_t ai;
	int64_t i;
	i = ati;
	ai = ati;
	EXPECT_EQ(ati, ai);
	ai = ai;
	EXPECT_EQ(ati, ai);

	amt::int16_t sh = 1;
	amt::uint16_t ush = 2;
	ush = sh;

	{
		amt::uint32_t ui = 1;
		amt::uint16_t ush = 2;
		amt::uint16_t ush2 = 5;
		if (ui + ush != ush2);
	}

	{
		unsigned int ui = 1;
		amt::uint32_t amt_ui = ui;
		unsigned long long ull = ui;
		amt::uint64_t amt_ull = amt_ui;
		EXPECT_EQ(ull, amt_ull);		
	}
}

void Free(int** pptr)
{
	if (*pptr)
	{
		free(*pptr);
		*pptr = nullptr;
	}
}

TEST(__AMT_TEST__, BasicPointerTest){
	amt::raw_ptr<int> ptr((int*) malloc(64));
	if (ptr)
	{
		for (size_t i = 0; i < 16; ++i)
			ptr[i] = 64;
		*ptr = 32;
		EXPECT_EQ(32 + *ptr + *(ptr + 1), 128);
		auto ptr2 = ptr + 1;
		EXPECT_EQ(*(ptr2 - 1), 32);
		Free(&ptr);
		EXPECT_EQ(ptr, nullptr);
	}
}

TEST(__AMT_TEST__, BasicArithmeticsTest){
	unsigned char x = 10;
	amt::uint8_t xx = 10;
	unsigned char y = 90;
	amt::uint8_t yy = 90;
	auto z = (x - y) * x;
	auto zz = (xx - yy) * xx;
	EXPECT_EQ(z, zz);
	auto plus = x + y;
	auto mul = (x * y) / 2;
	auto xmul = (x * yy) / 2;
	EXPECT_EQ(mul, xmul);

	amt::uint8_t a = 9;
	amt::uint8_t b = 11;
	amt::int32_t c = 0;
	c += (a - b) / 2;
	EXPECT_EQ(c, -1);
}

TEST(__AMT_TEST__, RemainingOperatorsTest) {
	short sh = 10;
	amt::int16_t ash(sh);

	sh <<= 3;
	ash <<= 3;
	EXPECT_EQ(sh, ash);
	if (sh)
	{
		sh >>= 1;		
	}
	if (ash)
	{
		ash >>= 1;		
	}
	EXPECT_EQ(sh, ash);
	sh ^= sh;	
	ash ^= ash;
	EXPECT_EQ(sh, ash);
	sh ^= 5ULL;
	ash ^= 5ULL;
	EXPECT_EQ(sh, ash);
}

TEST(__AMT_TEST__, LongLongTest){

	unsigned long long xll = 9;
	long long yll = 10;
	auto zll = (xll - yll) / 2;

	amt::uint64_t xxll = 9;
	amt::int64_t yyll = 10;
	amt::SetCustomAssertHandler<0>(&SilentCustomAssertHandler);
	auto counter = AssertionFailedSilently;
	auto zzll = (xxll - yyll) / 2;
	amt::SetCustomAssertHandler<0>(nullptr);
	#if AMTL_CHECK_NUMERIC_OVERFLOW_ON
	EXPECT_EQ(AssertionFailedSilently, counter + 1);
	#else
	EXPECT_EQ(AssertionFailedSilently, counter);
	#endif
	EXPECT_EQ(zll, zzll);

	yyll = 0;
	EXPECT_EQ(yyll + 1, 1);
}

TEST(__AMT_TEST__, LongLongOverflowTest){
	amt::uint64_t ll = 65536ULL * 65536 * 65536;
	auto ullmul = ll * 65535;
	amt::SetThrowCustomAssertHandler<0>();
	bool exceptionCaught = false;
	try{
		auto ullmul2 = ll * 65536;
	}
	catch (...)
	{
		exceptionCaught = true;
	}
	EXPECT_EQ(exceptionCaught, AMTL_CHECK_NUMERIC_OVERFLOW_ON);
}

TEST(__AMT_TEST__, LongLongAdditionTest) {

	unsigned long long ull = 10;
	long long ll = 0xFFFFFFFFFFFFFFULL;
	long long ll2 = -11;
	auto res = ull + ll;
	auto res2 = ull + ll2;

	amt::uint64_t aull = 10;
	amt::int64_t all = ll;
	amt::int64_t all2 = -11;

	auto ares = aull + all;

	amt::SetThrowCustomAssertHandler<0>();
	bool exceptionCaught = false;
	try
	{
		auto ares2 = aull + all2;
	}
	catch (...)
	{
		exceptionCaught = true;
	}
	EXPECT_EQ(exceptionCaught, AMTL_CHECK_NUMERIC_OVERFLOW_ON);
	EXPECT_EQ(res, ares);
	EXPECT_EQ((AreNumericTypesEquivalent<decltype(res), decltype(ares)>()), true);

	amt::uint64_t minull = 0;
	amt::int64_t small_negll = -1LL;
	exceptionCaught = false;
	try
	{
		auto overflow = minull + small_negll;
	}
	catch (...)
	{
		exceptionCaught = true;
	}
	EXPECT_EQ(exceptionCaught, AMTL_CHECK_NUMERIC_OVERFLOW_ON);

	exceptionCaught = false;
	try
	{
		short small_negsh = -1;
		auto overflow = minull + small_negsh;
	}
	catch (...)
	{
		exceptionCaught = true;
	}
	EXPECT_EQ(exceptionCaught, AMTL_CHECK_NUMERIC_OVERFLOW_ON);


}

TEST(__AMT_TEST__, LongLongSubtractionTest) {

	unsigned long long ull = 10;
	long long ll = -(long long)0xFFFFFFFFFFFFFFULL;
	long long ll2 = 11;
	auto res = ull - ll;
	auto res2 = ull - ll2;

	amt::uint64_t aull = ull;
	amt::int64_t all = ll;
	amt::int64_t all2 = ll2;

	auto ares = aull - all;

	amt::SetThrowCustomAssertHandler<0>();
	bool exceptionCaught = false;
	try
	{
		auto ares2 = aull - all2;
	}
	catch (...)
	{
		exceptionCaught = true;
	}
	EXPECT_EQ(exceptionCaught, AMTL_CHECK_NUMERIC_OVERFLOW_ON);
	EXPECT_EQ(res, ares);
	EXPECT_EQ((AreNumericTypesEquivalent<decltype(res), decltype(ares)>()), true);

	// --------------------
	exceptionCaught = false;
	amt::int64_t negll = (std::numeric_limits<long long>::min)() + 10;
	try
	{
		auto ares2 = negll - 9; 
	}
	catch (...)
	{
		exceptionCaught = true;
	}
	EXPECT_EQ(exceptionCaught, false);

	#ifndef __GNUC__ 
	negll = (std::numeric_limits<std::int64_t>::min)() + 10;
	try
	{
		int i = 15;
		auto ares2 = negll - i;
	}
	catch (...)
	{
		exceptionCaught = true;
	}
	EXPECT_EQ(exceptionCaught, AMTL_CHECK_NUMERIC_OVERFLOW_ON);
	#endif
		
	// -----------------------------------
	amt::uint64_t maxull = (std::numeric_limits<unsigned long long>::max)();
	amt::int64_t small_negll = -1LL;
	exceptionCaught = false;
	try
	{
		auto overflow = maxull - small_negll;
	}
	catch (...)
	{
		exceptionCaught = true;
	}
	EXPECT_EQ(exceptionCaught, AMTL_CHECK_NUMERIC_OVERFLOW_ON);

	exceptionCaught = false;
	try
	{
		short small_negsh = -1;
		auto overflow = maxull - small_negsh;
	}
	catch (...)
	{
		exceptionCaught = true;
	}
	EXPECT_EQ(exceptionCaught, AMTL_CHECK_NUMERIC_OVERFLOW_ON);

	short small_sh = 1;
	auto no_overflow = maxull - small_sh;
}

TEST(__AMT_TEST__, LongLongDivTest) {

	unsigned char uch = 100;
	unsigned char uch2 = 4;
	unsigned long long ull = 3;
	long long ll = 6;
	short sh = 7;
	auto res = uch / uch2;
	auto res2 = uch / ull;
	auto res3 = uch / ll;
	auto res4 = uch / sh;

	amt::uint8_t auch = 100;
	amt::uint8_t auch2 = 4;
	amt::uint64_t aull = 3;
	amt::int64_t all = 6;
	amt::int16_t ash = 7;

	auto ares = auch / auch2;
	auto ares2 = auch / aull;
	auto ares3 = auch / all;
	auto ares4 = auch / ash;

	EXPECT_EQ(res, ares);
	EXPECT_EQ(res2, ares2);
	EXPECT_EQ(res3, ares3);
	EXPECT_EQ(res4, ares4);

	EXPECT_EQ((AreNumericTypesEquivalent<decltype(res), decltype(ares)>()), true);
	EXPECT_EQ((AreNumericTypesEquivalent<decltype(res2), decltype(ares2)>()), true);
	EXPECT_EQ((AreNumericTypesEquivalent<decltype(res3), decltype(ares3)>()), true);
	EXPECT_EQ((AreNumericTypesEquivalent<decltype(res4), decltype(ares4)>()), true);
}

std::mt19937 mt;

// TODO: add a boolean parameter "generateMoreEdgeCases"
template<typename T>
T GetRandomFloat()
{
	static const T fMin = -(std::numeric_limits<T>::max)();
	static const T fMax = (std::numeric_limits<T>::max)();
	T f = ((T)mt()) / (std::mt19937::max)();
	return fMin + f * (fMax - fMin);
}

template<typename T>
T GetRandomInteger()
{
	if __AMT_IF_CONSTEXPR__(std::is_signed<T>::value)
	{
		if __AMT_IF_CONSTEXPR__(sizeof(T) > 4)
		{
			return (((T)mt()) << 32) + mt();
		}
		else
		{
			if (mt() % 2)
				return (T)mt();
			else
				return (T)(-(long long)mt());
		}
	}
	else
	{
		if __AMT_IF_CONSTEXPR__(sizeof(T) > 4)
		{
			return (((T)mt()) << 32) + mt();
		}
		else
		{
			return (T)mt();
		}
	}
}

template<typename T, bool isFloatingPoint = false>
struct GetRandomHelper
{
	static T get() { return GetRandomInteger<T>(); }
};
template<typename T>
struct GetRandomHelper<T, true>
{
	static T get() { return GetRandomFloat<T>(); }
};

template<typename T>
T GetRandom()
{
	return GetRandomHelper<T, std::is_floating_point<T>::value>::get();
}

template<typename T, typename U, bool anyFloat = std::is_floating_point<T>::value || std::is_floating_point<U>::value>
struct AvoidFloat
{
	//template<typename T, typename U>
	static void TestBitwiseOperators(T t, U u)
	{
		amt::AMTScalarType<T> tt(t);
		amt::AMTScalarType<U> uu(u);

		// Operator &
		auto And = t & u;
		auto amtAnd = tt & uu;
		auto amtAnd2 = tt & u;
		auto amtAnd3 = t & uu;

		EXPECT_EQ(And, amtAnd);
		EXPECT_EQ(And, amtAnd2);
		EXPECT_EQ(And, amtAnd3);
		EXPECT_EQ((AreNumericTypesEquivalent<decltype(And), decltype(amtAnd)>()), true);
		EXPECT_EQ((AreNumericTypesEquivalent<decltype(And), decltype(amtAnd2)>()), true);
		EXPECT_EQ((AreNumericTypesEquivalent<decltype(And), decltype(amtAnd3)>()), true);

		auto revand = u & t;
		auto amtRevAnd = uu & tt;
		auto amtRevAnd2 = u & tt;
		auto amtRevAnd3 = uu & t;
		EXPECT_EQ(revand, amtRevAnd);
		EXPECT_EQ(revand, amtRevAnd2);
		EXPECT_EQ(revand, amtRevAnd3);
		EXPECT_EQ((AreNumericTypesEquivalent<decltype(revand), decltype(amtRevAnd)>()), true);
		EXPECT_EQ((AreNumericTypesEquivalent<decltype(revand), decltype(amtRevAnd2)>()), true);
		EXPECT_EQ((AreNumericTypesEquivalent<decltype(revand), decltype(amtRevAnd3)>()), true);

		// Operator |
		auto Or = t | u;
		auto amtOr = tt | uu;
		auto amtOr2 = tt | u;
		auto amtOr3 = t | uu;

		EXPECT_EQ(Or, amtOr);
		EXPECT_EQ(Or, amtOr2);
		EXPECT_EQ(Or, amtOr3);
		EXPECT_EQ((AreNumericTypesEquivalent<decltype(Or), decltype(amtOr)>()), true);
		EXPECT_EQ((AreNumericTypesEquivalent<decltype(Or), decltype(amtOr2)>()), true);
		EXPECT_EQ((AreNumericTypesEquivalent<decltype(Or), decltype(amtOr3)>()), true);

		auto revor = u | t;
		auto amtRevOr = uu | tt;
		auto amtRevOr2 = u | tt;
		auto amtRevOr3 = uu | t;
		EXPECT_EQ(revor, amtRevOr);
		EXPECT_EQ(revor, amtRevOr2);
		EXPECT_EQ(revor, amtRevOr3);
		EXPECT_EQ((AreNumericTypesEquivalent<decltype(revor), decltype(amtRevOr)>()), true);
		EXPECT_EQ((AreNumericTypesEquivalent<decltype(revor), decltype(amtRevOr2)>()), true);
		EXPECT_EQ((AreNumericTypesEquivalent<decltype(revor), decltype(amtRevOr3)>()), true);

	}
};

template<typename T, typename U>
struct AvoidFloat<T, U, true>
{
	static void TestBitwiseOperators(T t, U u){}
};

// TODO: Test on const numbers also
template<typename T, typename U>
void TestScalarOperators()
{
	T t = GetRandom<T>();
	U u(GetRandom<U>());

	amt::AMTScalarType<T> tt(t);
	amt::AMTScalarType<U> uu(u);

	// Addition:
	auto sum = t + u;
	auto revsum = u + t;

	amt::SetCustomAssertHandler<0>(&SilentCustomAssertHandler);
	auto amtSum = tt + uu;
	auto amtRevSum = uu + tt;
	auto amtSum2 = tt + u;
	auto amtRevSum2 = u + tt;
	auto amtSum3 = t + uu;
	auto amtRevSum3 = uu + t;
	amt::SetCustomAssertHandler<0>(nullptr);

	if (sum == sum)
	{
		EXPECT_EQ(sum, amtSum);
		EXPECT_EQ(sum, amtSum2);
		EXPECT_EQ(sum, amtSum3);
	}
	else
	{
		EXPECT_NE(amtSum, amtSum);
		EXPECT_NE(amtSum2, amtSum2);
		EXPECT_NE(amtSum3, amtSum3);
	}
	if (revsum == revsum)
	{
		EXPECT_EQ(revsum, amtRevSum);
		EXPECT_EQ(revsum, amtRevSum2);
		EXPECT_EQ(revsum, amtRevSum3);
	}
	else
	{
		EXPECT_NE(amtRevSum, amtRevSum);
		EXPECT_NE(amtRevSum2, amtRevSum2);
		EXPECT_NE(amtRevSum3, amtRevSum3);
	}
	EXPECT_EQ((AreNumericTypesEquivalent<decltype(sum), decltype(amtSum)>()), true);
	EXPECT_EQ((AreNumericTypesEquivalent<decltype(sum), decltype(amtSum2)>()), true);
	EXPECT_EQ((AreNumericTypesEquivalent<decltype(sum), decltype(amtSum3)>()), true);
	EXPECT_EQ((AreNumericTypesEquivalent<decltype(revsum), decltype(amtRevSum)>()), true);
	EXPECT_EQ((AreNumericTypesEquivalent<decltype(revsum), decltype(amtRevSum2)>()), true);
	EXPECT_EQ((AreNumericTypesEquivalent<decltype(revsum), decltype(amtRevSum3)>()), true);

	// Subtraction:
	auto sub = t - u;
	auto revsub = u - t;

	amt::SetCustomAssertHandler<0>(&SilentCustomAssertHandler);
	auto amtlSub = tt - uu;
	auto amtlRevSub = uu - tt;
	auto amtlSub2 = tt - u;
	auto amtlRevSub2 = u - tt;
	auto amtlSub3 = t - uu;
	auto amtlRevSub3 = uu - t;
	amt::SetCustomAssertHandler<0>(nullptr);

	if (sub == sub)
	{
		EXPECT_EQ(sub, amtlSub);
		EXPECT_EQ(sub, amtlSub2);
		EXPECT_EQ(sub, amtlSub3);
	}
	else
	{
		EXPECT_NE(amtlSub, amtlSub);
		EXPECT_NE(amtlSub2, amtlSub2);
		EXPECT_NE(amtlSub3, amtlSub3);
	}
	if (revsub == revsub)
	{
		EXPECT_EQ(revsub, amtlRevSub);
		EXPECT_EQ(revsub, amtlRevSub2);
		EXPECT_EQ(revsub, amtlRevSub3);
	}
	else
	{
		EXPECT_NE(amtlRevSub, amtlRevSub);
		EXPECT_NE(amtlRevSub2, amtlRevSub2);
		EXPECT_NE(amtlRevSub3, amtlRevSub3);
	}
	EXPECT_EQ((AreNumericTypesEquivalent<decltype(sub), decltype(amtlSub)>()), true);
	EXPECT_EQ((AreNumericTypesEquivalent<decltype(sub), decltype(amtlSub2)>()), true);
	EXPECT_EQ((AreNumericTypesEquivalent<decltype(sub), decltype(amtlSub3)>()), true);
	EXPECT_EQ((AreNumericTypesEquivalent<decltype(revsub), decltype(amtlRevSub)>()), true);
	EXPECT_EQ((AreNumericTypesEquivalent<decltype(revsub), decltype(amtlRevSub2)>()), true);
	EXPECT_EQ((AreNumericTypesEquivalent<decltype(revsub), decltype(amtlRevSub3)>()), true);

	// Multiplication:
	auto mul = t * u;
	auto revmul = u * t;

	amt::SetCustomAssertHandler<0>(&SilentCustomAssertHandler);
	auto amtlMul = tt * uu;
	auto amtlRevMul = uu * tt;
	auto amtlMul2 = tt * u;
	auto amtlRevMul2 = u * tt;
	auto amtlMul3 = t * uu;
	auto amtlRevMul3 = uu * t;
	amt::SetCustomAssertHandler<0>(nullptr);

	if (mul == mul)
	{
		EXPECT_EQ(mul, amtlMul);
		EXPECT_EQ(mul, amtlMul2);
		EXPECT_EQ(mul, amtlMul3);
	}
	else
	{
		EXPECT_NE(amtlMul, amtlMul);
		EXPECT_NE(amtlMul2, amtlMul2);
		EXPECT_NE(amtlMul3, amtlMul3);
	}
	if (revmul == revmul)
	{
		EXPECT_EQ(revmul, amtlRevMul);
		EXPECT_EQ(revmul, amtlRevMul2);
		EXPECT_EQ(revmul, amtlRevMul3);
	}
	else
	{
		EXPECT_NE(amtlRevMul, amtlRevMul);
		EXPECT_NE(amtlRevMul2, amtlRevMul2);
		EXPECT_NE(amtlRevMul3, amtlRevMul3);
	}
	EXPECT_EQ((AreNumericTypesEquivalent<decltype(mul), decltype(amtlMul)>()), true);
	EXPECT_EQ((AreNumericTypesEquivalent<decltype(mul), decltype(amtlMul2)>()), true);
	EXPECT_EQ((AreNumericTypesEquivalent<decltype(mul), decltype(amtlMul3)>()), true);
	EXPECT_EQ((AreNumericTypesEquivalent<decltype(revmul), decltype(amtlRevMul)>()), true);
	EXPECT_EQ((AreNumericTypesEquivalent<decltype(revmul), decltype(amtlRevMul2)>()), true);
	EXPECT_EQ((AreNumericTypesEquivalent<decltype(revmul), decltype(amtlRevMul3)>()), true);

	// Division:
	if ((t != 0 && u != 0) || std::is_floating_point<T>::value || std::is_floating_point<U>::value)
	{
		auto div = t / u;
		auto revdiv = u / t;

		amt::SetCustomAssertHandler<0>(&SilentCustomAssertHandler);
		auto amtlDiv = tt / uu;
		auto amtlRevDiv = uu / tt;
		auto amtlDiv2 = tt / u;
		auto amtlRevDiv2 = u / tt;
		auto amtlDiv3 = t / uu;
		auto amtlRevDiv3 = uu / t;
		amt::SetCustomAssertHandler<0>(nullptr);

		if (div == div)
		{
			EXPECT_EQ(div, amtlDiv);
			EXPECT_EQ(div, amtlDiv2);
			EXPECT_EQ(div, amtlDiv3);
		}
		else
		{
			EXPECT_NE(amtlDiv, amtlDiv);
			EXPECT_NE(amtlDiv2, amtlDiv2);
			EXPECT_NE(amtlDiv3, amtlDiv3);
		}
		if (revdiv == revdiv)
		{
			EXPECT_EQ(revdiv, amtlRevDiv);
			EXPECT_EQ(revdiv, amtlRevDiv2);
			EXPECT_EQ(revdiv, amtlRevDiv3);
		}
		else
		{
			EXPECT_NE(amtlRevDiv, amtlRevDiv);
			EXPECT_NE(amtlRevDiv2, amtlRevDiv2);
			EXPECT_NE(amtlRevDiv3, amtlRevDiv3);
		}
		EXPECT_EQ((AreNumericTypesEquivalent<decltype(div), decltype(amtlDiv)>()), true);
		EXPECT_EQ((AreNumericTypesEquivalent<decltype(div), decltype(amtlDiv2)>()), true);
		EXPECT_EQ((AreNumericTypesEquivalent<decltype(div), decltype(amtlDiv3)>()), true);
		EXPECT_EQ((AreNumericTypesEquivalent<decltype(revdiv), decltype(amtlRevDiv)>()), true);
		EXPECT_EQ((AreNumericTypesEquivalent<decltype(revdiv), decltype(amtlRevDiv2)>()), true);
		EXPECT_EQ((AreNumericTypesEquivalent<decltype(revdiv), decltype(amtlRevDiv3)>()), true);
	}

	AvoidFloat<T, U, (std::is_floating_point<T>::value || std::is_floating_point<U>::value)> ::TestBitwiseOperators(t, u);
}

template<typename T>
void TestScalarOperators()
{
	for (size_t i = 0; i < 256; ++i)
	{
		TestScalarOperators<T, char>();
		TestScalarOperators<T, unsigned char>();
		TestScalarOperators<T, short>();
		TestScalarOperators<T, unsigned short>();
		TestScalarOperators<T, int>();
		TestScalarOperators<T, unsigned int>();
		TestScalarOperators<T, long>(); // let's run also on this "polimorphic" type alias
		TestScalarOperators<T, unsigned long>();
		TestScalarOperators<T, long long>();
		TestScalarOperators<T, unsigned long long>();
		TestScalarOperators<T, float>();
		TestScalarOperators<T, double>();
		TestScalarOperators<T, long double>();
	}
}

TEST(__AMT_TEST__, ScalarOperatorsStressTest)
{	
	// TODO: maybe seed Mersenne Twister...	
	TestScalarOperators<char>();
	TestScalarOperators<unsigned char>();
	TestScalarOperators<short>();
	TestScalarOperators<unsigned short>();
	TestScalarOperators<int>();
	TestScalarOperators<unsigned int>();
	TestScalarOperators<long>(); // let's run also on this "polimorphic" type alias
	TestScalarOperators<unsigned long>();
	TestScalarOperators<long long>();
	TestScalarOperators<unsigned long long>();
	TestScalarOperators<float>();
	TestScalarOperators<double>();
	TestScalarOperators<long double>();	
}

bool basicVectorTestAssertionFailure = false;
void BasicVectorTest_CustomAssertHandler(bool a, const char* szFileName, long lLine, const char* szDesc)
{
	if (!a)
		basicVectorTestAssertionFailure = true;
}

template<typename VectorType>
void BasicVectorTestImpl()
{
	VectorType vec;
	//std::vector<int> vec;
	EXPECT_EQ(vec.size(), 0);
	EXPECT_EQ(vec.capacity(), 0);
	vec.reserve(32);
	EXPECT_EQ(vec.size(), 0);
	EXPECT_GE(vec.capacity(), 32);
	vec.emplace_back(10);
	EXPECT_EQ(vec.size(), 1);
	vec = vec;
	EXPECT_EQ(vec.size(), 1);
	auto it = vec.begin();
	EXPECT_NE(it, vec.end());

	vec.push_back(7);
	vec.push_back(5);
	EXPECT_EQ(vec.size(), 3);
	EXPECT_EQ(vec[0], 10);
	//amt::vector<int>::iterator i1 = vec.begin();
	//amt::vector<int>::iterator i2 = vec.end();
	//std::sort(i1, i2);
	std::sort(vec.begin(), vec.end());
	EXPECT_EQ(vec[0], 5);

	std::sort(vec.rbegin(), vec.rend());
	EXPECT_EQ(vec[0], 10);

	it = vec.begin();
	it += 3;
	it -= 2;
	--it;
	EXPECT_EQ(it, vec.begin());

	auto diff = vec.end() - it;
	EXPECT_EQ(diff, vec.size());

	EXPECT_EQ(it[0], 10);
	EXPECT_EQ((it++)[1], 7);
	EXPECT_EQ((it--)[0], 7);
	EXPECT_EQ(*++it, 7);
	EXPECT_EQ(it[-1], 10);
	if (!std::is_same<VectorType, std::vector<int>>::value)
	{
		size_t sum = 0;
		basicVectorTestAssertionFailure = false;
		amt::SetCustomAssertHandler<0>(&BasicVectorTest_CustomAssertHandler);
		sum += it[-2];
		EXPECT_EQ(basicVectorTestAssertionFailure, true);
		basicVectorTestAssertionFailure = false;
		sum += it[3];
		EXPECT_EQ(basicVectorTestAssertionFailure, true);
		EXPECT_EQ(sum, 0);
		amt::SetCustomAssertHandler<0>(nullptr);
	}

	auto itNewlyAdded = vec.insert(vec.end(), { 1, 1, 1 });
	EXPECT_EQ(vec.size(), 6);
	EXPECT_EQ(*itNewlyAdded++, 1);
	EXPECT_EQ(*itNewlyAdded++, 1);
	EXPECT_EQ(*itNewlyAdded++, 1);
	itNewlyAdded = vec.insert(vec.begin() + 1, 3);
	EXPECT_EQ(*itNewlyAdded++, 3);
	EXPECT_NE(*itNewlyAdded++, 3);
	EXPECT_EQ(vec.size(), 7);
}

TEST(__AMT_TEST__, BasicVectorTest) 
{
	BasicVectorTestImpl<std::vector<int>>();
	BasicVectorTestImpl<amt::vector<int>>();

	// Test cross-type iterator implicit cast: 
	amt::vector<int> vec;
	vec.push_back(10);
	vec.push_back(5);
	std::vector<int>::iterator it = vec.begin(); // implicitly cast amt::vector::iterator to std::vector iterator
	EXPECT_EQ(*it, 10);

	std::sort<std::vector<int>::iterator>(it, vec.end()); // mixing iterators unfortunately requires explicit type specification
	EXPECT_EQ(*it, 5); // formally iterators are invalidated but in practice std::vector iterators should be ok
}

struct SomeStruct
{
	std::vector<int> data_;
	bool operator < (const SomeStruct& o) const
	{
		return data_ < o.data_;
	}
};

template<typename MapType>
void BasicMapTestImpl()
{
	MapType map;
	EXPECT_EQ(map.size(), 0);
	EXPECT_EQ(map.find(0), map.end());
	map[0] = 0;
	EXPECT_EQ(map.size(), 1);
	map = map;
	EXPECT_EQ(map.size(), 1);
	map[1] = 1;
	EXPECT_EQ(map.size(), 2);
	map[1] = 2;
	EXPECT_EQ(map.size(), 2);
	EXPECT_EQ(map.empty(), false);
	auto it = map.find(0);
	EXPECT_NE(it, map.end());
	EXPECT_EQ(it->first, 0);
	EXPECT_EQ(it->second, 0);

	it->second = 3;
	EXPECT_EQ(map[0], 3);

	typename MapType::const_iterator cit(it);
	EXPECT_EQ(cit->first, 0);
	EXPECT_EQ(cit->second, 3);

	map.insert(std::make_pair(5, 25));
	EXPECT_EQ(map.size(), 3);
	map.insert(map.end(), std::make_pair(6, 36));
	EXPECT_EQ(map.size(), 4);

	map.insert( { {10, 100}, {20, 400} });
	EXPECT_EQ(map.size(), 6);
	
	it = map.end();
	EXPECT_EQ((--it)->second, 400);
	EXPECT_EQ((it--)->second, 400);
	EXPECT_EQ(it->second, 100);

	map = { {0, 0}, {3, 3} };
	EXPECT_EQ(map.size(), 2);
}

TEST(__AMT_TEST__, BasicMapTest) 
{
	BasicMapTestImpl<std::map<int, int>>();
	BasicMapTestImpl<amt::map<int, int>>();

	amt::map<int, SomeStruct> omap;
	omap.insert(std::move(std::make_pair(1, SomeStruct())));
	EXPECT_EQ(omap.size(), 1);

	amt::map<int, int> map;
	amt::map<int, int>::const_iterator amtConstIt = map.begin();
	std::map<int, int>::const_iterator nativeConstIt = map.begin();
}

template<typename SetType>
void BasicSetTestImpl()
{
	SetType set;
	EXPECT_EQ(set.size(), 0);
	EXPECT_EQ(set.find(0), set.end());

	set.insert(0);
	EXPECT_EQ(set.size(), 1);
	set = set;
	EXPECT_EQ(set.size(), 1);

	set.insert(1);
	EXPECT_EQ(set.size(), 2);
	auto it = set.find(0);
	EXPECT_NE(it, set.end());
	EXPECT_EQ(*it, 0);

	set.insert(42);
	it = set.end();
	EXPECT_EQ(*--it, 42);
	EXPECT_EQ(*it--, 42);
	EXPECT_EQ(*it--, 1);
	EXPECT_EQ(*it++, 0);
	EXPECT_EQ(*it, 1);
	EXPECT_EQ(*it++, 1);
	EXPECT_EQ(*it, 42);

	set = { 42, 41, 4 };
	EXPECT_EQ(set.size(), 3);
	EXPECT_EQ(*set.begin(), 4);
	EXPECT_EQ(*set.rbegin(), 42);

	auto lbIt = set.lower_bound(42);
	EXPECT_EQ(*lbIt, 42);
	auto ubIt = set.upper_bound(42);
	EXPECT_EQ(ubIt, set.end());
}

TEST(__AMT_TEST__, BasicSetTest) 
{
	BasicSetTestImpl<std::set<int>>();
	BasicSetTestImpl<amt::set<int>>();

	amt::set<SomeStruct> oset;
	oset.insert(SomeStruct());
	EXPECT_EQ(oset.size(), 1);
}

// -------------------------------------------------------

class Employee
{
	std::string name;
	int id;
public:
	Employee(const std::string& name, size_t id) : name(name), id(id)
	{
	}
	std::string& getName()
	{
		return name;
	}
	const std::string& getName() const
	{
		return name;
	}
	size_t getId() const
	{
		return id;
	}
};

struct EmployeeComparer
{
	using is_transparent = void;
	bool operator()(Employee const& employee1, Employee const& employee2) const
	{
		return employee1.getId() < employee2.getId();
	}
	bool operator()(int id, Employee const& employee) const
	{
		return id < employee.getId();
	}
	bool operator()(Employee const& employee, int id) const
	{
		return employee.getId() < id;
	}
};


template<class SetType>
void LessBasicSetTestImpl()
{
	SetType set{ {"Peter", 1}, {"Anna", 2}};
	set.emplace("John", 3);
	set.emplace("Joanna", 4);

	#if __AMT_CPP14ORHIGHER__
	auto it = set.find(2);
	EXPECT_EQ(it->getName(), "Anna");

	it = set.lower_bound(3);
	EXPECT_EQ(it->getName(), "John");

	it = set.upper_bound(2);
	EXPECT_EQ(it->getName(), "John");

	auto res = set.equal_range(1);
	auto it1 = res.first;
	auto it2 = res.second;	
	EXPECT_EQ(it1->getName(), "Peter");
	EXPECT_EQ(it2->getName(), "Anna");
	#endif
}

TEST(__AMT_TEST__, LessBasicSetTest)
{
	LessBasicSetTestImpl<std::set<Employee, EmployeeComparer>>();
	LessBasicSetTestImpl<amt::set<Employee, EmployeeComparer>>();
}

// ======================================================================
//  Group of tests for unsynchronized access
// ----------------------------------------------------------------------
// Test unsynchronized access to integer

bool IntUnsynchWriteTest_AssertionFailed = false;

void IntUnsynchWriteTest_CustomAssertHandler(bool a, const char* szFileName, long lLine, const char* szDesc)
{
	if (!a)
		if (strstr(szDesc, "m_nPendingWriteRequests") != nullptr) // make sure this is the assertion we expect		
			IntUnsynchWriteTest_AssertionFailed = true;
}

void IntUnsynchWriteTest_WriterThread(size_t threadNo, amt::int32_t& val, std::atomic<bool>& canStartThread)
{
	while (!canStartThread); // make sure threads start at the same time
	for (size_t i = 0; i < 65536UL * 4 && !IntUnsynchWriteTest_AssertionFailed; ++i)
		if (threadNo)
			++val;
		else
			--val;
	return;
}

TEST(__AMT_TEST__, IntUnsynchWriteTest) {
	srand(time(NULL));
	amt::SetCustomAssertHandler<0>(&IntUnsynchWriteTest_CustomAssertHandler);
	amt::int32_t val;
	std::atomic<bool> canStartThread(false);
	std::thread thread1(&IntUnsynchWriteTest_WriterThread, 0, std::ref(val), std::ref(canStartThread));
	std::thread thread2(&IntUnsynchWriteTest_WriterThread, 1, std::ref(val), std::ref(canStartThread));
	canStartThread = true;
	thread1.join();
	thread2.join();
	EXPECT_EQ(IntUnsynchWriteTest_AssertionFailed, AMTL_MAIN_FEATURE_ON);
	amt::SetCustomAssertHandler<0>(nullptr);
}

// ----------------------------------------------------------------------
// Test vector for synchronized access when writing. Expected no assertion failure.

bool VectorSynchWriteTest_AssertionFailed = false;
std::recursive_mutex mtxVectorSynchWriteTest;

void VectorSynchWriteTest_CustomAssertHandler(bool a, const char* szFileName, long lLine, const char* szDesc)
{
	if (!a)
		if (strstr(szDesc, "m_nPendingWriteRequests == 0") != nullptr) // make sure this is the assertion we expect		
			VectorSynchWriteTest_AssertionFailed = true;
}

void VectorSynchWriteTest_WriterThread(size_t threadNo, amt::vector<int>& vec)
{
	for (size_t i = 0; i < 32678 && !VectorSynchWriteTest_AssertionFailed; ++i)
	{
		std::unique_lock<std::recursive_mutex> lock(mtxVectorSynchWriteTest);
		vec.push_back(i);
	}
	return;
}
inline size_t GetCurrentSize(const amt::vector<int>& vec)
{
	std::unique_lock<std::recursive_mutex> lock(mtxVectorSynchWriteTest);
	return vec.size();
}
void VectorSynchWriteTest_ReaderThread(size_t threadNo, amt::vector<int>& vec)
{
	size_t size = GetCurrentSize(vec);
	for (size_t i = 0; i < 32678 && !VectorSynchWriteTest_AssertionFailed; ++i)
	{
		std::unique_lock<std::recursive_mutex> lock(mtxVectorSynchWriteTest);
		if (size)
		{
			size_t idx = rand() % size;
			++vec[idx];
		}
		size = GetCurrentSize(vec);
	}
	return;
}

TEST(__AMT_TEST__, VectorSynchWriteTest) {
	srand(time(NULL));
	amt::SetCustomAssertHandler<0>(&VectorSynchWriteTest_CustomAssertHandler);
	amt::vector<int> vec;
	std::thread thread1(&VectorSynchWriteTest_WriterThread, 0, std::ref(vec));
	std::thread thread2(&VectorSynchWriteTest_ReaderThread, 1, std::ref(vec));
	thread1.join();
	thread2.join();
	EXPECT_EQ(VectorSynchWriteTest_AssertionFailed, false);
	amt::SetCustomAssertHandler<0>(nullptr);
}

// ----------------------------------------------------------------------
// Test vector for unsynchronized access when writing. Expected assertion failure.

bool VectorUnsynchWriteTest_AssertionFailed = false;

void VectorUnsynchWriteTest_CustomAssertHandler(bool a, const char* szFileName, long lLine, const char* szDesc)
{
	if (!a)
	{
		if (strstr(szDesc, "m_nPendingWriteRequests == 0") != nullptr) // make sure this is the assertion we expect		
			VectorUnsynchWriteTest_AssertionFailed = true;

#if defined(__AMT_TEST_WITHOUT_GTEST__) && defined(_WIN32)
		//MessageBoxA(nullptr, "Assertion failed", "Assertion failed", MB_OK);
#endif
	}
}

void VectorUnsynchWriteTest_WriterThread(size_t threadNo, amt::vector<int>& vec, std::atomic<bool>& canStartThread)
{
	while (!canStartThread); // make sure threads start at the same time
	for (size_t i = 0; i < 65536UL * 32 && !VectorUnsynchWriteTest_AssertionFailed; ++i)
		vec.push_back(i);
	return;
}
void VectorUnsynchWriteTest_ReaderThread(size_t threadNo, amt::vector<int>& vec, std::atomic<bool>& canStartThread)
{
	while (!canStartThread); // make sure threads start at the same time
	for (size_t i = 0; i < 65536UL * 32 && !VectorUnsynchWriteTest_AssertionFailed; ++i)
	{
		if (vec.size())
		{
			size_t idx = rand() % vec.size();
			++vec[idx];
		}
	}
	return;
}


TEST(__AMT_TEST__, VectorUnsynchWriteTest) {
	#if AMTL_MAIN_FEATURE_ON
	srand(time(NULL));
	amt::SetCustomAssertHandler<0>(&VectorUnsynchWriteTest_CustomAssertHandler);
	amt::vector<int> vec;
	std::atomic<bool> canStartThread(false);
	std::thread thread1(&VectorUnsynchWriteTest_WriterThread, 0, std::ref(vec), std::ref(canStartThread));
	std::thread thread2(&VectorUnsynchWriteTest_ReaderThread, 1, std::ref(vec), std::ref(canStartThread));
	canStartThread = true;
	thread1.join();
	thread2.join();
	EXPECT_EQ(VectorUnsynchWriteTest_AssertionFailed, true);
	amt::SetCustomAssertHandler<0>(nullptr);
	#endif
}

struct TestPodStruct
{
	int i;
	double db;
};

TEST(__AMT_TEST__, VectorInitializationTest) {

	amt::SetCustomAssertHandler<0>(nullptr);

	amt::vector<double> vecDbl{ 1.0, 1.0, 2.0, 3.0, 5.0, 8.0, 13.0, 21.0 };
	EXPECT_EQ(vecDbl.size(), 8);
	EXPECT_EQ(vecDbl[vecDbl.size() - 1], 21.0);

	amt::vector<int> vecInt{ 1, 1, 2, 3, 5, 8, 13, 21, 34 };
	EXPECT_EQ(vecInt.size(), 9);
	EXPECT_EQ(vecInt[vecInt.size() - 1], 34);

	amt::vector<amt::AMTScalarType<double>> vecDbl2{ 1.0, 1.0, 2.0, 3.0, 5.0, 8.0, 13.0, 21.0, 34.0, 55.0 };
	EXPECT_EQ(vecDbl2.size(), 10);
	EXPECT_EQ(vecDbl2[vecDbl2.size() - 1], 55.0);

	amt::vector<amt::AMTScalarType<int>> vecInt2{ 1, 1, 2, 3, 5, 8, 13, 21, 34, 55, 89 };
	EXPECT_EQ(vecInt2.size(), 11);
	EXPECT_EQ(vecInt2[vecInt2.size() - 1], 89);

	amt::vector<double> vecZerosDbl(64);
	amt::vector<amt::AMTScalarType<double>> vecZerosDbl2(64);
	amt::vector<int> vecZerosInt(64);
	amt::vector<amt::AMTScalarType<int>> vecZerosInt2(64);

	EXPECT_EQ(vecZerosDbl.size(), 64);
	EXPECT_EQ(vecZerosDbl2.size(), 64);
	EXPECT_EQ(vecZerosInt.size(), 64);
	EXPECT_EQ(vecZerosInt2.size(), 64);
	EXPECT_EQ(std::count_if(vecZerosDbl.begin(), vecZerosDbl.end(), [](double db) {return db != 0.0; }), 0);
	EXPECT_EQ(std::count_if(vecZerosDbl2.begin(), vecZerosDbl2.end(), [](double db){return db != 0.0; }), 0);
	EXPECT_EQ(std::count_if(vecZerosInt.begin(), vecZerosInt.end(), [](int i){return i != 0; }), 0);
	EXPECT_EQ(std::count_if(vecZerosInt2.begin(), vecZerosInt2.end(), [](int i){return i != 0; }), 0);

	amt::vector<double> otherVecDbl(vecZerosDbl.begin(), vecZerosDbl.end());
	amt::vector<double> yetAnotherVecDbl;
	yetAnotherVecDbl.assign(vecZerosDbl.begin(), vecZerosDbl.end());
	amt::vector<amt::AMTScalarType<double>> otherVecDbl2(vecZerosDbl2.begin(), vecZerosDbl2.end());
	amt::vector<int> otherVecInt(vecZerosInt.begin(), vecZerosInt.end());
	amt::vector<amt::AMTScalarType<int>> otherVecInt2(vecZerosInt2.begin(), vecZerosInt2.end());

	EXPECT_EQ(otherVecDbl.size(), 64);
	EXPECT_EQ(otherVecDbl2.size(), 64);
	EXPECT_EQ(otherVecInt.size(), 64);
	EXPECT_EQ(otherVecInt2.size(), 64);
	EXPECT_EQ(std::count_if(otherVecDbl.begin(), otherVecDbl.end(), [](double db){return db != 0.0; }), 0);
	EXPECT_EQ(std::count_if(otherVecDbl2.begin(), otherVecDbl2.end(), [](double db){return db != 0.0; }), 0);
	EXPECT_EQ(std::count_if(otherVecInt.begin(), otherVecInt.end(), [](int i){return i != 0; }), 0);
	EXPECT_EQ(std::count_if(otherVecInt2.begin(), otherVecInt2.end(), [](int i){return i != 0; }), 0);

	std::vector<TestPodStruct> vecPod(64);
	amt::vector<TestPodStruct> amtVecPod(64);
	//EXPECT_EQ(vecPod, amtVecPod);
	EXPECT_EQ(vecPod.size(), amtVecPod.size());
	for (size_t i = 0; i < vecPod.size(); ++i)
	{
		EXPECT_EQ(vecPod[i].i, amtVecPod[i].i);
		EXPECT_EQ(vecPod[i].db, amtVecPod[i].db);
	}

}

// ----------------------------------------------------------------------
// Test map for unsynchronized access when writing. Expected assertion failure.

bool MapUnsynchWriteTest_AssertionFailed = false;
std::atomic<size_t> mapUnsynchWriteTest_WriterThreads_started{0};

void MapUnsynchWriteTest_CustomAssertHandler(bool a, const char* szFileName, long lLine, const char* szDesc)
{
	if (!a)
		if (strstr(szDesc, "m_nPendingWriteRequests") != nullptr) // make sure this is the assertion we expect		
			MapUnsynchWriteTest_AssertionFailed = true;
}

void MapUnsynchWriteTest_WriterThread(size_t threadNo, amt::map<int, int>& map, std::atomic<bool>& canStartThread)
{
	++ mapUnsynchWriteTest_WriterThreads_started;
	while (!canStartThread); // make sure threads start at the same time
	
	size_t iStart = threadNo ? 65536 : 0;
	size_t iEnd = threadNo ? 65536 * 2 : 65536;
	
	for (size_t i = 0 ; i < 16 ; ++i)	
		for (size_t i = iStart; i < iEnd && !MapUnsynchWriteTest_AssertionFailed; ++i)
			map[i] = i + threadNo;	
}

TEST(__AMT_TEST__, MapUnsynchWriteTest) {
	mapUnsynchWriteTest_WriterThreads_started = 0;
	srand(time(NULL));
	amt::SetCustomAssertHandler<0>(&MapUnsynchWriteTest_CustomAssertHandler);
	amt::map<int, int> _map;
	#if !defined(__AMTL_ASSERTS_ARE_ON__) || !__AMT_CHECK_MULTITHREADED_ISSUES__
	for (size_t i = 0; i < 65536 * 2; ++i)
		_map[i] = 0; // fill in in advance, so that the test doesn't crash when the check is off
	#endif
	std::atomic<bool> canStartThread(false);
	std::thread thread1(&MapUnsynchWriteTest_WriterThread, 0, std::ref(_map), std::ref(canStartThread));
	std::thread thread2(&MapUnsynchWriteTest_WriterThread, 1, std::ref(_map), std::ref(canStartThread));
	while (mapUnsynchWriteTest_WriterThreads_started < 2);
	canStartThread = true;
	thread1.join();
	thread2.join();
	EXPECT_EQ(MapUnsynchWriteTest_AssertionFailed, AMTL_MAIN_FEATURE_ON);
	amt::SetCustomAssertHandler<0>(nullptr);
}

TEST(__AMT_TEST__, MapInitializationTest){
	amt::map<int, std::string> map{ { 1, "1" }, { 2, "2" } };
	EXPECT_EQ(map.size(), 2);
	EXPECT_EQ(map[1], "1");
	EXPECT_EQ((std::is_same<decltype(map)::mapped_type, std::string>::value), true);
	map[AMTL_OPTIONS_HASH + 55] = "AMTL options";

	amt::map<int, std::string> map2(map.begin(), map.end());
	EXPECT_EQ(map2[2], "2");
	EXPECT_EQ(map2.size(), 3);
}

// ----------------------------------------------------------------------
// Test set for unsynchronized access when writing. Expected assertion failure.

TEST(__AMT_TEST__, SetInitializationTest)
{
	amt::set<std::string> set{ "1", "2", "22" };
	EXPECT_EQ(set.size(), 3);
	EXPECT_EQ(*set.begin(), "1");
	EXPECT_EQ(*set.rbegin(), "22");
	EXPECT_EQ((std::is_same<decltype(set)::value_type, std::string>::value), true);
	set.insert(std::to_string(AMTL_OPTIONS_HASH + 55));

	amt::set<std::string> set2(set.begin(), set.end());
	EXPECT_EQ(set2.size(), 4);
}

bool SetUnsynchWriteTest_AssertionFailed = false;

void SetUnsynchWriteTest_CustomAssertHandler(bool a, const char* szFileName, long lLine, const char* szDesc)
{
	if (!a)
		if (strstr(szDesc, "m_nPendingWriteRequests") != nullptr) // make sure this is the assertion we expect		
			SetUnsynchWriteTest_AssertionFailed = true;
}

void SetUnsynchWriteTest_WriterThread(size_t threadNo, amt::set<int>& set, std::atomic<bool>& canStartThread)
{
	while (!canStartThread); // make sure threads start at the same time
	size_t iStart = threadNo ? 65536 : 0;
	size_t iEnd = threadNo ? 65536 * 2 : 65536;
	for (size_t i = iStart; i < iEnd && !SetUnsynchWriteTest_AssertionFailed; ++i)
		set.insert(i);
}

TEST(__AMT_TEST__, SetUnsynchWriteTest) {

	srand(time(NULL));
	amt::SetCustomAssertHandler<0>(&SetUnsynchWriteTest_CustomAssertHandler);
	amt::set<int> set;
	#if !defined(__AMTL_ASSERTS_ARE_ON__) || !__AMT_CHECK_MULTITHREADED_ISSUES__
	for (size_t i = 0; i < 65536; ++i)
		set.insert(i); // fill in in advance, so that the test doesn't crash when the check is off
	#endif
	std::atomic<bool> canStartThread(false);
	std::thread thread1(&SetUnsynchWriteTest_WriterThread, 0, std::ref(set), std::ref(canStartThread));
	std::thread thread2(&SetUnsynchWriteTest_WriterThread, 1, std::ref(set), std::ref(canStartThread));
	canStartThread = true;
	thread1.join();
	thread2.join();
	EXPECT_EQ(SetUnsynchWriteTest_AssertionFailed, AMTL_MAIN_FEATURE_ON);
	amt::SetCustomAssertHandler<0>(nullptr);
}

// =================================================================================================
// Group of test for invalid iterators
// ----------------------------------------------------------------------------
// Tests of iterators of amt::set

TEST(__AMT_TEST__, SetCheckIteratorValidityTest) {
	bool assertionFailed = false;
	amt::SetThrowCustomAssertHandler<0>();
	amt::set<int> set;

	amt::set<int>::const_iterator cit = set.begin();
	if (cit == set.end()); //these are just the checks.. 
	EXPECT_EQ(cit, set.end()); //...that this code compiles

	auto it = set.begin();
	it = it;
	EXPECT_EQ(it, it);

	try{
		#if AMTL_CHECK_ITERATORS_VALIDITY_ON
		++it; // cannot increment on end
		#endif
	}
	catch (...)
	{
		assertionFailed = true;
	}
	EXPECT_EQ(assertionFailed, AMTL_CHECK_ITERATORS_VALIDITY_ON);
}

TEST(__AMT_TEST__, SetCheckIteratorValidityTest_2) {
	bool assertionFailed = false;
	amt::SetThrowCustomAssertHandler<0>();
	amt::set<int> set;
	auto it = set.begin();
	try
	{
		#if AMTL_CHECK_ITERATORS_VALIDITY_ON
		--it; // cannot decrement on begin
		#endif
	}
	catch (...)
	{
		assertionFailed = true;
	}
	EXPECT_EQ(assertionFailed, AMTL_CHECK_ITERATORS_VALIDITY_ON);
}

TEST(__AMT_TEST__, SetCheckIteratorValidityTest_3) {
	bool assertionFailed = false;
	amt::SetThrowCustomAssertHandler<0>();
	amt::set<int> set;
	auto it = set.begin();
	set.insert(1); // invalidates it
	bool isEnd = false;
	try
	{
		isEnd = it == set.end();
	}
	catch (...)
	{
		assertionFailed = true;
	}

	EXPECT_EQ(assertionFailed, AMTL_CHECK_ITERATORS_VALIDITY_ON);
}

TEST(__AMT_TEST__, SetCheckIteratorValidityTest_4) {
	bool assertionFailed = false;
	amt::SetThrowCustomAssertHandler<0>();
	amt::set<int> set;
	amt::set<int> set2;
	auto it = set.begin();
	bool isEnd = false;
	try
	{
		isEnd = it == set2.end(); // cannot use it of set vs end() of different object (set2)
	}
	catch (...)
	{
		assertionFailed = true;
	}
	EXPECT_EQ(assertionFailed, AMTL_CHECK_ITERATORS_VALIDITY_ON);
}

// ---------------------------------------------------------------------
// Test unsynchronized update of an iterator in different threads

bool SetIter_UnsynchUpdateTest_AssertionFailed = false;
std::atomic<size_t> SetIter_UnsynchUpdateTest_ThreadsStarted;

void SetIter_UnsynchUpdateTest_CustomAssertHandler(bool a, const char* szFileName, long lLine, const char* szDesc)
{
	if (!a)
		if (strstr(szDesc, "m_nPendingWriteRequests") != nullptr) // make sure this is the assertion we expect
			SetIter_UnsynchUpdateTest_AssertionFailed = true;
}

void SetIter_UnsynchUpdateTest_WriterThread(size_t threadNo, amt::set<int>& set, amt::set<int>::iterator& it, std::atomic<bool>& canStartThread)
{
	auto origIt = it;
	++ SetIter_UnsynchUpdateTest_ThreadsStarted;
	while (!canStartThread); // make sure threads start at the same time			

	try
	{		
		for(size_t i = 0 ; i < 16 ; ++ i)
		{
			if (threadNo)
			{
				while (it != set.end())
					++it;
			}
			else
				while (it != set.begin())
					--it;
			it = origIt;
		}		
	}
	catch (amt::AMTCassertException& e)
	{
		SetIter_UnsynchUpdateTest_AssertionFailed = true;
	}
	catch (...)
	{
		SetIter_UnsynchUpdateTest_AssertionFailed = true;
	}
	return;
}

TEST(__AMT_TEST__, SetIter_UnsynchUpdateTest) {

	SetIter_UnsynchUpdateTest_ThreadsStarted = 0;
	amt::SetCustomAssertHandler<0>(&SetIter_UnsynchUpdateTest_CustomAssertHandler);
	amt::set<int> set;
	for (int i = 0; i < 65536; ++i)
		set.insert(i);
	auto it = set.find(32768);
	std::atomic<bool> canStartThread(false);
	std::thread thread1(&SetIter_UnsynchUpdateTest_WriterThread, 0, std::ref(set), std::ref(it), std::ref(canStartThread));
	std::thread thread2(&SetIter_UnsynchUpdateTest_WriterThread, 1, std::ref(set), std::ref(it), std::ref(canStartThread));	
	while(SetIter_UnsynchUpdateTest_ThreadsStarted < 2);
	canStartThread = true;

	thread1.join();
	thread2.join();
	EXPECT_EQ(SetIter_UnsynchUpdateTest_AssertionFailed, AMTL_CHECK_SYNC_OF_ACCESS_TO_ITERATORS_ON);
	amt::SetCustomAssertHandler<0>(nullptr);
}

// ----------------------------------------------------------------------------
// Tests of iterators of amt::map

TEST(__AMT_TEST__, MapCheckIteratorValidityTest) {
	bool assertionFailed = false;
	amt::SetThrowCustomAssertHandler<0>();

	amt::map<int, int> map;
	amt::map<int, int>::const_iterator cit = map.begin();
	if (cit == map.end()); //these are just the checks.. 
	EXPECT_EQ(cit, map.end()); //...that this code compiles	

	auto it = map.begin();
	it = it;
	EXPECT_EQ(it, it);

	#if __AMT_CHECK_ITERATORS_VALIDITY__ || !defined(__clang__)
	try{
		++it; // cannot increment on end
	}
	catch (...)
	{
		assertionFailed = true;
	}
	EXPECT_EQ(assertionFailed, AMTL_CHECK_ITERATORS_VALIDITY_ON);
	#endif
}

TEST(__AMT_TEST__, MapCheckIteratorValidityTest_2) {
	bool assertionFailed = false;
	amt::SetThrowCustomAssertHandler<0>();
	amt::map<int, int> map;
	auto it = map.begin();

	#if __AMT_CHECK_ITERATORS_VALIDITY__ || !defined(__clang__)
	try
	{
		--it; // cannot decrement on begin
	}
	catch (...)
	{
		assertionFailed = true;
	}
	EXPECT_EQ(assertionFailed, AMTL_CHECK_ITERATORS_VALIDITY_ON);
	#endif
}

TEST(__AMT_TEST__, MapCheckIteratorValidityTest_3) {
	bool assertionFailed = false;
	amt::SetThrowCustomAssertHandler<0>();
	amt::map<int, int> map;
	auto it = map.begin();
	map[1] = 1; // invalidates it
	bool isEnd = false;

	#if __AMT_CHECK_ITERATORS_VALIDITY__ || !defined(__clang__)
	try
	{
		isEnd = it == map.end();
	}
	catch (...)
	{
		assertionFailed = true;
	}

	EXPECT_EQ(assertionFailed, AMTL_CHECK_ITERATORS_VALIDITY_ON);
	#endif
}

TEST(__AMT_TEST__, MapCheckIteratorValidityTest_4) {
	bool assertionFailed = false;
	amt::SetThrowCustomAssertHandler<0>();
	amt::map<int, int> map;
	amt::map<int, int> map2;
	auto it = map.begin();
	bool isEnd = false;

	#if __AMT_CHECK_ITERATORS_VALIDITY__ || !defined(__clang__)
	try
	{
		isEnd = it == map2.end(); // cannot use it of set vs end() of different object (set2)
	}
	catch (...)
	{
		assertionFailed = true;
	}
	EXPECT_EQ(assertionFailed, AMTL_CHECK_ITERATORS_VALIDITY_ON);
	#endif
}

// ---------------------------------------------------------------------
// Test unsynchronized update of an iterator in different threads

bool MapIter_UnsynchUpdateTest_AssertionFailed = false;

void MapIter_UnsynchUpdateTest_CustomAssertHandler(bool a, const char* szFileName, long lLine, const char* szDesc)
{
	if (!a)
		if (strstr(szDesc, "m_nPendingWriteRequests") != nullptr) // make sure this is the assertion we expect
			MapIter_UnsynchUpdateTest_AssertionFailed = true;
}

std::atomic<size_t> MapIter_UnsynchUpdateTest_WriterThreads_started{0};

void MapIter_UnsynchUpdateTest_WriterThread(size_t threadNo, amt::map<int, int>& map, amt::map<int, int>::iterator& it, std::atomic<bool>& canStartThread)
{
	auto origIt = it;
	++ MapIter_UnsynchUpdateTest_WriterThreads_started;
	while (!canStartThread); // make sure threads start at the same time			

	try
	{
		for(size_t i = 0 ; i < 16 ; ++ i)
		{
			if (threadNo)
			{
				while (it != map.end())
					++it;
			}
			else
				while (it != map.begin())
					--it;
			it = origIt;	
		}
	}
	catch (amt::AMTCassertException& e)
	{
		MapIter_UnsynchUpdateTest_AssertionFailed = true;
	}
	catch (...)
	{
		MapIter_UnsynchUpdateTest_AssertionFailed = true;
	}
	return;
}

TEST(__AMT_TEST__, MapIter_UnsynchUpdateTest) {

	MapIter_UnsynchUpdateTest_WriterThreads_started = 0;
	amt::SetCustomAssertHandler<0>(&MapIter_UnsynchUpdateTest_CustomAssertHandler);
	amt::map<int, int> map;
	for (int i = 0; i < 2 * 65536; ++i)
		map[i] = i;
	auto it = map.find(2 * 32768);
	std::atomic<bool> canStartThread(false);
	std::thread thread1(&MapIter_UnsynchUpdateTest_WriterThread, 0, std::ref(map), std::ref(it), std::ref(canStartThread));
	std::thread thread2(&MapIter_UnsynchUpdateTest_WriterThread, 1, std::ref(map), std::ref(it), std::ref(canStartThread));
	while (MapIter_UnsynchUpdateTest_WriterThreads_started < 2);
	canStartThread = true;

	thread1.join();
	thread2.join();
	EXPECT_EQ(MapIter_UnsynchUpdateTest_AssertionFailed, AMTL_CHECK_SYNC_OF_ACCESS_TO_ITERATORS_ON);
	amt::SetCustomAssertHandler<0>(nullptr);
}

// ===================================================================================================================
// Group of tests for TObjectRawDataDebugChecker - a class that asserts that some data stays intact/not overwritten meanwhile
// -----------------------------------------------------------

TEST(__AMT_TEST__, TestObjectRawDataDebugChecker) {
	amt::SetThrowCustomAssertHandler<0>();
	bool assertionFailed = false;

	try
	{
		char ch = ' ';
		amt::TObjectRawDataDebugChecker<char> a(&ch);
		ch = 'C';
	}
	catch (amt::AMTCassertException& e)
	{
		if (strstr(e.sDesc.c_str(), "DataHasChanged") != nullptr)
			assertionFailed = true;
	}
	EXPECT_EQ(assertionFailed, AMTL_ON);
}

TEST(__AMT_TEST__, TestObjectRawDataDebugChecker_2) {
	amt::SetThrowCustomAssertHandler<0>();
	bool assertionFailed = false;

	try
	{
		struct MyData
		{
			char data[1024];
			MyData()
			{
				memset(data, 0, 1024);
			}
		};

		MyData myData;
		amt::TObjectRawDataDebugChecker<MyData> a(&myData);
		myData.data[512] = 'C';
	}
	catch (amt::AMTCassertException& e)
	{
		if (strstr(e.sDesc.c_str(), "DataHasChanged") != nullptr)
			assertionFailed = true;
	}
	EXPECT_EQ(assertionFailed, AMTL_ON);
}

TEST(__AMT_TEST__, TestObjectRawDataDebugChecker_AllOK) {
	amt::SetThrowCustomAssertHandler<0>();
	bool assertionFailed = false;

	try
	{
		struct MyData
		{
			char data[1024];
			MyData()
			{
				memset(data, 0, 1024);
			}
		};

		MyData myData;
		amt::TObjectRawDataDebugChecker<MyData> a(&myData);
		// no change in data
	}
	catch (amt::AMTCassertException& e)
	{
		assertionFailed = true;
	}
	EXPECT_EQ(assertionFailed, false);
}


// =================================================================================================
// Group of tests for numeric overflow
// ----------------------------------------------------------------------------
// Tests for char (amt::_char or amt::int8_t)

TEST(__AMT_TEST__, CharNumericOverflowTest_AllOK) {
	bool assertionFailed = false;
	amt::SetThrowCustomAssertHandler<0>();
	amt::int8_t ch = 1;
	try
	{
		ch *= 5;
		auto backup(ch);
		ch |= ch;
		ch &= ch;
		ch += ch;
		ch *= ch;
		ch /= ch;
		ch %= ch;
		ch -= ch;
		ch = backup;
		ch += 55;

		amt::int8_t o = ch * 2.1;
		EXPECT_EQ((std::uint8_t) o, 126);

		amt::int8_t o2(ch * 2.1);
		EXPECT_EQ((std::uint8_t) o2, 126);

		ch /= 10;
		ch -= 6 + 128;
		ch = 0;
		++ch;
		ch++;
		ch = ch * 1.5;
		ch *= 1.7;

		EXPECT_EQ((std::uint8_t) ch, 5);
		ch = 6 % ch;
	}
	catch (amt::AMTCassertException& e)
	{
		assertionFailed = true;
	}
	EXPECT_EQ(assertionFailed, false);
}

TEST(__AMT_TEST__, CharNumericOverflowTest_Add) {
	bool assertionFailed = false;
	amt::SetThrowCustomAssertHandler<0>();
	amt::int8_t ch = 100;
	try
	{
		ch += 28; // overflow; char max is 127
	}
	catch (amt::AMTCassertException& e)
	{
		assertionFailed = true;
	}
	EXPECT_EQ(assertionFailed, AMTL_CHECK_NUMERIC_OVERFLOW_ON);
}

TEST(__AMT_TEST__, CharNumericOverflowTest_Inc) {
	bool assertionFailed = false;
	amt::SetThrowCustomAssertHandler<0>();
	amt::int8_t ch = 127;
	try
	{
		++ch; // overflow; char max is 127
	}
	catch (amt::AMTCassertException& e)
	{
		assertionFailed = true;
	}
	EXPECT_EQ(assertionFailed, AMTL_CHECK_NUMERIC_OVERFLOW_ON);
}

TEST(__AMT_TEST__, CharNumericOverflowTest_PostInc) {
	bool assertionFailed = false;
	amt::SetThrowCustomAssertHandler<0>();
	amt::int8_t ch = 127;
	try
	{
		ch++; // overflow; char max is 127
	}
	catch (amt::AMTCassertException& e)
	{
		assertionFailed = true;
	}
	EXPECT_EQ(assertionFailed, AMTL_CHECK_NUMERIC_OVERFLOW_ON);
}

TEST(__AMT_TEST__, CharNumericOverflowTest_Subtract_1) {
	bool assertionFailed = false;
	amt::SetThrowCustomAssertHandler<0>();
	amt::int8_t ch = -100;
	try
	{
		ch -= 28; // no overflow; char min is -128
	}
	catch (amt::AMTCassertException& e)
	{
		assertionFailed = true;
	}
	EXPECT_EQ(assertionFailed, false);	
}

TEST(__AMT_TEST__, CharNumericOverflowTest_Subtract_2) {
	bool assertionFailed = false;
	amt::SetThrowCustomAssertHandler<0>();
	amt::int8_t ch = -100;
	try
	{
		ch -= 29; // overflow; char min is -128
	}
	catch (amt::AMTCassertException& e)
	{
		assertionFailed = true;
	}
	EXPECT_EQ(assertionFailed, AMTL_CHECK_NUMERIC_OVERFLOW_ON);
}

TEST(__AMT_TEST__, CharNumericOverflowTest_Subtract_3) {
	bool assertionFailed = false;
	amt::SetThrowCustomAssertHandler<0>();
	amt::int8_t ch = -100;
	amt::int32_t i = 250;
	
	try
	{
		ch -= i;
	}
	catch (amt::AMTCassertException& e)
	{
		assertionFailed = true;
	}
	EXPECT_EQ(assertionFailed, AMTL_CHECK_NUMERIC_OVERFLOW_ON);	
}

TEST(__AMT_TEST__, CharNumericOverflowTest_Subtract_4) {
	bool assertionFailed = false;
	amt::SetThrowCustomAssertHandler<0>();
	amt::int8_t ch = -100;
	amt::int8_t ch_sub = 28;
	
	try
	{
		ch -= ch_sub;
	}
	catch (amt::AMTCassertException& e)
	{
		assertionFailed = true;
	}
	EXPECT_EQ(assertionFailed, false);	
	
	ch = -100;
	ch_sub = 29;
	try
	{
		ch -= ch_sub;
	}
	catch (amt::AMTCassertException& e)
	{
		assertionFailed = true;
	}	
	EXPECT_EQ(assertionFailed, AMTL_CHECK_NUMERIC_OVERFLOW_ON);	
}

TEST(__AMT_TEST__, CharNumericOverflowTest_Dec) {
	bool assertionFailed = false;
	amt::SetThrowCustomAssertHandler<0>();
	amt::int8_t ch = -128;
	try
	{
		--ch; // overflow; char min is -128
	}
	catch (amt::AMTCassertException& e)
	{
		assertionFailed = true;
	}
	EXPECT_EQ(assertionFailed, AMTL_CHECK_NUMERIC_OVERFLOW_ON);
}

TEST(__AMT_TEST__, CharNumericOverflowTest_PostDec) {
	bool assertionFailed = false;
	amt::SetThrowCustomAssertHandler<0>();
	amt::int8_t ch = -128;
	try
	{
		ch--; // overflow; char min is -128
	}
	catch (amt::AMTCassertException& e)
	{
		assertionFailed = true;
	}
	EXPECT_EQ(assertionFailed, AMTL_CHECK_NUMERIC_OVERFLOW_ON);
}

TEST(__AMT_TEST__, CharNumericOverflowTest_Mul) {
	bool assertionFailed = false;
	amt::SetThrowCustomAssertHandler<0>();
	amt::int8_t ch = 50;
	try
	{
		ch *= 3; // overflow
	}
	catch (amt::AMTCassertException& e)
	{
		assertionFailed = true;
	}
	EXPECT_EQ(assertionFailed, AMTL_CHECK_NUMERIC_OVERFLOW_ON);
}

TEST(__AMT_TEST__, CharNumericOverflowTest_Div) {
	bool assertionFailed = false;
	amt::SetThrowCustomAssertHandler<0>();
	amt::int8_t ch = -128;
	try
	{
		ch /= -1; // overflow; char max is 127
	}
	catch (amt::AMTCassertException& e)
	{
		assertionFailed = true;
	}
	EXPECT_EQ(assertionFailed, AMTL_CHECK_NUMERIC_OVERFLOW_ON);
}

TEST(__AMT_TEST__, CharNumericOverflowTest_DivFloat) {
	bool assertionFailed = false;
	amt::SetThrowCustomAssertHandler<0>();
	amt::int8_t ch = 1;
	try
	{
		ch /= -0.005; // overflow; char min is -128
	}
	catch (amt::AMTCassertException& e)
	{
		assertionFailed = true;
	}
	EXPECT_EQ(assertionFailed, AMTL_CHECK_NUMERIC_OVERFLOW_ON);
}

TEST(__AMT_TEST__, CharNumericOverflowTest_DivZero) {
	#if AMTL_CHECK_NUMERIC_OVERFLOW_ON
	bool assertionFailed = false;
	amt::SetThrowCustomAssertHandler<0>();
	amt::int8_t ch = 1;
	try
	{
		ch /= 0;
	}
	catch (amt::AMTCassertException& e)
	{
		assertionFailed = true;
	}
	EXPECT_EQ(assertionFailed, AMTL_CHECK_NUMERIC_OVERFLOW_ON);
	#endif
}

// ------------------------------------------------------
// Tests for unsigned char (amt::uint8_t)

TEST(__AMT_TEST__, UCharNumericOverflowTest_AllOK) {
	bool assertionFailed = false;
	amt::SetThrowCustomAssertHandler<0>();
	amt::uint8_t uch = 1;
	try
	{
		uch *= 5;
		uch += 55;

		amt::uint8_t o = uch * 4.2;
		EXPECT_EQ((std::uint8_t) o, 252);

		uch /= 10;
		uch -= 6;
		++uch;
		uch++;
		uch = uch * 1.5;
		uch *= 1.7;

		EXPECT_EQ((std::uint8_t) uch, 5);
	}
	catch (amt::AMTCassertException& e)
	{
		assertionFailed = true;
	}
	EXPECT_EQ(assertionFailed, false);
}

TEST(__AMT_TEST__, UCharNumericOverflowTest_Add) {
	bool assertionFailed = false;
	amt::SetThrowCustomAssertHandler<0>();
	amt::uint8_t uch = 254;
	try
	{
		uch += 2; // numeric overflow; unsigned char max is 255
	}
	catch (amt::AMTCassertException& e)
	{
		assertionFailed = true;
	}
	EXPECT_EQ(assertionFailed, AMTL_CHECK_NUMERIC_OVERFLOW_ON);
}

TEST(__AMT_TEST__, UCharNumericOverflowTest_Inc) {
	bool assertionFailed = false;
	amt::SetThrowCustomAssertHandler<0>();
	amt::uint8_t uch = 255;
	try
	{
		++uch; // numeric overflow; unsigned char max is 255
	}
	catch (amt::AMTCassertException& e)
	{
		assertionFailed = true;
	}
	EXPECT_EQ(assertionFailed, AMTL_CHECK_NUMERIC_OVERFLOW_ON);
}

TEST(__AMT_TEST__, UCharNumericOverflowTest_PostInc) {
	bool assertionFailed = false;
	amt::SetThrowCustomAssertHandler<0>();
	amt::uint8_t uch = 255;
	try
	{
		uch++; // numeric overflow; unsigned char max is 255
	}
	catch (amt::AMTCassertException& e)
	{
		assertionFailed = true;
	}
	EXPECT_EQ(assertionFailed, AMTL_CHECK_NUMERIC_OVERFLOW_ON);
}

TEST(__AMT_TEST__, UCharNumericOverflowTest_Sub) {
	bool assertionFailed = false;
	amt::SetThrowCustomAssertHandler<0>();
	amt::uint8_t uch = 5;
	try
	{
		uch -= 10; // numeric overflow; unsigned char min is 0
	}
	catch (amt::AMTCassertException& e)
	{
		assertionFailed = true;
	}
	EXPECT_EQ(assertionFailed, AMTL_CHECK_NUMERIC_OVERFLOW_ON);
}

TEST(__AMT_TEST__, UCharNumericOverflowTest_Dec) {
	bool assertionFailed = false;
	amt::SetThrowCustomAssertHandler<0>();
	amt::uint8_t uch = 0;
	try
	{
		--uch; // numeric overflow; unsigned char min is 0
	}
	catch (amt::AMTCassertException& e)
	{
		assertionFailed = true;
	}
	EXPECT_EQ(assertionFailed, AMTL_CHECK_NUMERIC_OVERFLOW_ON);
}

TEST(__AMT_TEST__, UCharNumericOverflowTest_PostDec) {
	bool assertionFailed = false;
	amt::SetThrowCustomAssertHandler<0>();
	amt::uint8_t uch = 0;
	try
	{
		uch--; // numeric overflow; unsigned char min is 0
	}
	catch (amt::AMTCassertException& e)
	{
		assertionFailed = true;
	}
	EXPECT_EQ(assertionFailed, AMTL_CHECK_NUMERIC_OVERFLOW_ON);
}

TEST(__AMT_TEST__, UCharNumericOverflowTest_Mul_fine) {
	bool assertionFailed = false;
	amt::SetThrowCustomAssertHandler<0>();
	amt::uint8_t uch = 51;
	try
	{
		uch *= 5; // ok, unsigned char max is 255
	}
	catch (amt::AMTCassertException& e)
	{
		assertionFailed = true;
	}
	EXPECT_EQ(assertionFailed, false);
}

TEST(__AMT_TEST__, UCharNumericOverflowTest_Mul) {
	bool assertionFailed = false;
	amt::SetThrowCustomAssertHandler<0>();
	amt::uint8_t uch = 16;
	try
	{
		uch *= 16; // assertion failure, unsigned char max is 255
	}
	catch (amt::AMTCassertException& e)
	{
		assertionFailed = true;
	}
	EXPECT_EQ(assertionFailed, AMTL_CHECK_NUMERIC_OVERFLOW_ON);
}

TEST(__AMT_TEST__, UCharNumericOverflowTest_MulFloat) {
	bool assertionFailed = false;
	amt::SetThrowCustomAssertHandler<0>();
	amt::uint8_t uch = 100;
	try
	{
		uch *= 2.6; // assertion failure, unsigned char max is 255
	}
	catch (amt::AMTCassertException& e)
	{
		assertionFailed = true;
	}
	EXPECT_EQ(assertionFailed, AMTL_CHECK_NUMERIC_OVERFLOW_ON);
}

TEST(__AMT_TEST__, UCharNumericOverflowTest_MulNeg) {
	bool assertionFailed = false;
	amt::SetThrowCustomAssertHandler<0>();
	amt::uint8_t uch = 16;
	try
	{
		uch *= -1; // assertion failure
	}
	catch (amt::AMTCassertException& e)
	{
		assertionFailed = true;
	}
	EXPECT_EQ(assertionFailed, AMTL_CHECK_NUMERIC_OVERFLOW_ON);
}

TEST(__AMT_TEST__, UCharNumericOverflowTest_Div) {
	bool assertionFailed = false;
	amt::SetThrowCustomAssertHandler<0>();
	amt::uint8_t uch = 10;
	try
	{
		uch /= -1; // assertion failure
	}
	catch (amt::AMTCassertException& e)
	{
		assertionFailed = true;
	}
	EXPECT_EQ(assertionFailed, AMTL_CHECK_NUMERIC_OVERFLOW_ON);
	assertionFailed = false; // reset this flag
	try
	{
		uch = uch / -1;
	}
	catch (amt::AMTCassertException& e)
	{
		assertionFailed = true;
	}
	EXPECT_EQ(assertionFailed, AMTL_CHECK_NUMERIC_OVERFLOW_ON);
}

TEST(__AMT_TEST__, UCharNumericOverflowTest_DivFloat) {
	bool assertionFailed = false;
	amt::SetThrowCustomAssertHandler<0>();
	amt::uint8_t uch = 100;
	try
	{
		uch /= 0.3; // assertion failure
	}
	catch (amt::AMTCassertException& e)
	{
		assertionFailed = true;
	}
	EXPECT_EQ(assertionFailed, AMTL_CHECK_NUMERIC_OVERFLOW_ON);
}

TEST(__AMT_TEST__, UCharNumericOverflowTest_DivZero) {
	#if AMTL_CHECK_NUMERIC_OVERFLOW_ON
	bool assertionFailed = false;
	amt::SetThrowCustomAssertHandler<0>();
	amt::uint8_t uch = 10;
	try
	{
		uch /= 0; // assertion failure
	}
	catch (amt::AMTCassertException& e)
	{
		assertionFailed = true;
	}
	EXPECT_EQ(assertionFailed, AMTL_CHECK_NUMERIC_OVERFLOW_ON);
	#endif
}

TEST(__AMT_TEST__, UCharRestFromDivision) {
	bool assertionFailed = false;
	amt::SetThrowCustomAssertHandler<0>();
	amt::uint8_t uch = 10;
	try
	{
		uch %= 4;
		EXPECT_EQ(uch, 2);
		#if AMTL_CHECK_NUMERIC_OVERFLOW_ON
		uch %= 0;
		#endif
	}
	catch (amt::AMTCassertException& e)
	{
		assertionFailed = true;
	}
	EXPECT_EQ(assertionFailed, AMTL_CHECK_NUMERIC_OVERFLOW_ON);
	assertionFailed = false; // reset this flag
	try
	{
		#if AMTL_CHECK_NUMERIC_OVERFLOW_ON
		uch = uch % 0;
		#endif
	}
	catch (amt::AMTCassertException& e)
	{
		assertionFailed = true;
	}
	EXPECT_EQ(assertionFailed, AMTL_CHECK_NUMERIC_OVERFLOW_ON);

}

// ------------------------------------------------------
// Tests for double (amt::_double)

TEST(__AMT_TEST__, DoubleNumericOverflowTest_AllOK) {
	bool assertionFailed = false;
	amt::SetThrowCustomAssertHandler<0>();
	amt::_double db = 1000000.0;
	try
	{
		db *= 5.0;
		db += 55.0;

		double o = db * 4.2;
		EXPECT_EQ(o, 5000055 * 4.2);

		o /= 10;
		EXPECT_EQ(o, 5000055 * 4.2 / 10.0);
		o -= 6;
		EXPECT_EQ(o, 5000055 * 4.2 / 10.0 - 6);
		++o;
		EXPECT_EQ(o, 5000055 * 4.2 / 10.0 - 6 + 1);
		o++;
		EXPECT_EQ(o, 5000055 * 4.2 / 10.0 - 6 + 2);
		o = o * 1.5;
		EXPECT_EQ(o, (5000055 * 4.2 / 10.0 - 6 + 2) * 1.5);
		o *= 1.7;
		EXPECT_EQ(o, (5000055 * 4.2 / 10.0 - 6 + 2) * 1.5 * 1.7);
	}
	catch (amt::AMTCassertException& e)
	{
		assertionFailed = true;
	}
	EXPECT_EQ(assertionFailed, false);
}

TEST(__AMT_TEST__, DoubleCorrectArithmeticsTest)
{
	/*double db = 5.25;
	amt::_double adb = db;
	amt::int32_t ai = 2;

	auto res = 2 * db;
	auto amt_res = ai * adb;
	EXPECT_EQ(res, amt_res);
	*/

	bool assertionFailed = false;
	amt::SetThrowCustomAssertHandler<0>();

	amt::int32_t i(2.5);
	amt::int32_t i2(0);
	amt::uint32_t ui(0);
	amt::uint16_t ush(0);
	amt::int16_t sh(0);
	amt::_double db(1);

	amt::int32_t two(2);
	amt::_double adb2(1.1);
	double db2(adb2);
	auto plus = 2 + (2 + db2) * (2 - db2) + db2;
	auto aplus = two + (two + adb2) * (two - adb2) + adb2;
	EXPECT_EQ(aplus, plus);

	auto times = 2 / (2 * db2) / (2 * db2) / db2;
	auto atimes = two / (two * adb2) / (two * adb2) / adb2;
	EXPECT_EQ(atimes, times);

	try
	{
		amt::int32_t i4(3000000000.0);
	}
	catch (amt::AMTCassertException& e)
	{
		assertionFailed = true;
	}
	EXPECT_EQ(assertionFailed, AMTL_CHECK_NUMERIC_OVERFLOW_ON);

	assertionFailed = false; // reset
	ui = 100000;
	try
	{
		ush = ui;
	}
	catch (amt::AMTCassertException& e)
	{
		assertionFailed = true;
	}
	EXPECT_EQ(assertionFailed, AMTL_CHECK_NUMERIC_OVERFLOW_ON);
}

TEST(__AMT_TEST__, NumericLimitsTest)
{
	EXPECT_EQ((std::numeric_limits<amt::uint8_t>::max)(), 255);
	EXPECT_EQ((std::numeric_limits<amt::uint8_t>::min)(), 0);

	EXPECT_EQ(std::numeric_limits<amt::uint8_t>::is_signed, false);
	EXPECT_EQ(std::numeric_limits<amt::int64_t>::is_signed, true);

	EXPECT_EQ(std::numeric_limits<amt::uint8_t>::is_integer, true);
	EXPECT_EQ(std::numeric_limits<amt::int64_t>::is_integer, true);
	EXPECT_EQ(std::numeric_limits<amt::_double>::is_integer, false);
	EXPECT_EQ(std::numeric_limits<amt::_float>::is_integer, false);

	EXPECT_EQ(std::numeric_limits<amt::uint8_t>::is_exact, true);
	EXPECT_EQ(std::numeric_limits<amt::_double>::is_exact, false);
	EXPECT_EQ(std::numeric_limits<amt::_float>::is_exact, false);

	EXPECT_EQ(std::numeric_limits<amt::uint8_t>::has_infinity, false);
	EXPECT_EQ(std::numeric_limits<amt::_double>::has_infinity, true);
	EXPECT_EQ(std::numeric_limits<amt::_float>::has_infinity, true);

	EXPECT_EQ(std::numeric_limits<amt::uint8_t>::digits, 8);
	EXPECT_EQ(std::numeric_limits<amt::int8_t>::digits, 7);

}

TEST(__AMT_TEST__, EmplaceTest)
{
	struct Struct
	{
		std::vector<int> vec_;
		Struct(std::vector<int>&& vec) : vec_(vec) {};
		size_t size() const { return vec_.size(); }
		bool operator < (const Struct& o) const
		{
			return size() < o.size();
		}
	};

	std::vector<Struct> stdVec;
	auto stdIt = stdVec.emplace(stdVec.begin(), std::vector<int>{1, 2});

	amt::vector<Struct> vec;
	vec.emplace_back(std::vector<int>{ 1 });
	EXPECT_EQ(vec.size(), 1);
	EXPECT_EQ(vec[0].size(), 1);
	
	auto begin = vec.begin();	
	amt::vector<Struct>::const_iterator constBeginIter = vec.begin();
	auto rbegin = vec.rbegin();
	amt::vector<Struct>::const_reverse_iterator constRBeginIter = vec.rbegin();

	auto it = vec.emplace(vec.begin(), std::vector<int>{1, 2});
	EXPECT_NE(it, vec.end());
	EXPECT_EQ(vec.size(), 2);

	it = vec.begin();
	auto& data = *(it + 1);
	++it;
	auto& data2 = *(it - 1);
	it -= 1;

	amt::map<int, Struct> map;
	map.emplace(1, std::vector<int>{1, 2, 3});
	auto mit = map.find(1);
	EXPECT_EQ(mit->second.size(), 3);

	amt::set<Struct> set;
	set.emplace(std::vector<int>{1, 2, 3, 4});
	auto sit = set.begin();
	EXPECT_EQ(sit->size(), 4);

}

struct StructForAMTPointerTypeTest
{
	char ch;
	int i;
	double db;
};
TEST(__AMT_TEST__, AMTPointerTypeTest)
{
	StructForAMTPointerTypeTest* raw_ptr = new StructForAMTPointerTypeTest[12];
	amt::AMTPointerType<StructForAMTPointerTypeTest*> ptr = raw_ptr;
	ptr->ch = 'A';
	ptr->i = 1;
	ptr->db = 2.5;
	const StructForAMTPointerTypeTest* const_raw_ptr = raw_ptr;
	amt::AMTPointerType<const StructForAMTPointerTypeTest*> cptr = ptr;
	EXPECT_EQ(cptr, ptr);
	EXPECT_EQ(cptr->i, ptr->i);
	for (size_t i = 1; i < 12; ++ i)
		ptr[i] = ptr[i - 1];
	delete[] ptr;
}

TEST(__AMT_TEST__, VectorOfAMTDoubleInitializationTest)
{
	std::vector<amt::AMTScalarType<double>> vec(5);
	EXPECT_EQ(0.0, vec[0]);
	EXPECT_EQ(0.0, vec[1]);
	EXPECT_EQ(0.0, vec[2]);
	EXPECT_EQ(0.0, vec[3]);
	EXPECT_EQ(0.0, vec[4]);
}	

TEST(__AMT_TEST__, AMTVectorOfAMTDoubleInitializationTest)
{
	amt::vector<amt::AMTScalarType<double>> amt_vec(5);
	EXPECT_EQ(0.0, amt_vec[0]);
	EXPECT_EQ(0.0, amt_vec[1]);
	EXPECT_EQ(0.0, amt_vec[2]);
	EXPECT_EQ(0.0, amt_vec[3]);
	EXPECT_EQ(0.0, amt_vec[4]);		
}

// ----------------------------------------------------------------------

TEST(__AMT_TEST__, AMTWStringBasicTest)
{
	amt::wstring str(L"str");
	EXPECT_EQ(str.length(), 3);
	EXPECT_EQ(str[1], L't');
	str += str;
	EXPECT_EQ(str.size(), 6);
	EXPECT_EQ(str, L"strstr");
	str.push_back(' ');
	EXPECT_EQ(str.size(), 7);
	EXPECT_EQ(str, L"strstr ");
	str.pop_back();
	str.pop_back();
	EXPECT_EQ(str.size(), 5);
	EXPECT_EQ(str, L"strst");
	str = str.substr(1, 2);
	EXPECT_EQ(str.size(), 2);
	EXPECT_EQ(str, L"tr");

	amt::wstring str2(str);
	EXPECT_EQ(str2.size(), 2);
	EXPECT_EQ(str2, L"tr");

	amt::wstring str3 = std::move(str2);
	EXPECT_EQ(str3.size(), 2);
	EXPECT_EQ(str3, L"tr");

	amt::wstring str4(str3.begin(), str3.end());
	EXPECT_EQ(str4.size(), 2);
	EXPECT_EQ(str4, L"tr");
	EXPECT_EQ(str3, str4);
	EXPECT_EQ(str3, str4.c_str());

	auto it = str4.begin();
	auto itEnd = str4.end();
	EXPECT_NE(it, itEnd);
}

TEST(__AMT_TEST__, AMTWStringIteratorValidityTest)
{
	bool assertionFailed = false;
	amt::SetThrowCustomAssertHandler<0>();
	amt::wstring str;

	auto cit = str.begin();
	if (cit == str.end()); //these are just the checks.. 
	EXPECT_EQ(cit, str.end()); //...that this code compiles

	auto it = str.begin();
	it = it;
	EXPECT_EQ(it, it);

	try {
		#if AMTL_CHECK_ITERATORS_VALIDITY_ON
		++it; // cannot increment on end
		#endif
	}
	catch (...)
	{
		assertionFailed = true;
	}
	EXPECT_EQ(assertionFailed, AMTL_CHECK_ITERATORS_VALIDITY_ON);

	assertionFailed = false;
	amt::wstring str2;
	try {
		#if AMTL_CHECK_ITERATORS_VALIDITY_ON
		if (it == str2.begin()) // cannot compare iterators of different instances
			str2.push_back(L' ');
		#endif
	}
	catch (...)
	{
		assertionFailed = true;
	}
	EXPECT_EQ(assertionFailed, AMTL_CHECK_ITERATORS_VALIDITY_ON);

	assertionFailed = false;
	it = str2.end();
	str2.push_back(' '); // invalidates iterator
	try {
		#if AMTL_CHECK_ITERATORS_VALIDITY_ON
		++ it; // cannot use an invalidated operator
		#endif
	}
	catch (...)
	{
		assertionFailed = true;
	}

	EXPECT_EQ(assertionFailed, AMTL_CHECK_ITERATORS_VALIDITY_ON);

	str2 = L"abc";
	it = str2.begin();
	EXPECT_EQ(*it, L'a');
	++it;
	EXPECT_EQ(*it, L'b');
	++it;
	EXPECT_EQ(*it, L'c');
	++it;
	EXPECT_EQ(it, str2.end());
}

std::atomic<size_t> amtWStringErrorsCount{ 0 };

void AMTWStringUnsynchWriteTest_CustomAssertHandler(bool isAssertionOk, const char* szFileName, long lLine, const char* szDesc)
{
	if (!isAssertionOk)
	{
		++amtWStringErrorsCount;
	}
}

void AMTWStringUnsyncUpdateThread(amt::wstring& s, std::atomic<size_t>& threadsStarted, std::atomic<bool>& canStartThreadWork)
{
	++threadsStarted;
	while (!canStartThreadWork);

	auto len = s.size();
	for (size_t i = 0 ; i < 65536 * 4 && amtWStringErrorsCount == 0 ; ++ i)
	{
		for (size_t i = 0; i < len; ++i)
			s[i] = i; // unsynchronized write to a string				
	}
}

TEST(__AMT_TEST__, AMTWStringUnsyncUpdate)
{
	amt::wstring s(L"abcdefghijklmnopqrstuvwxyz");
	std::atomic<size_t> threadsStarted{ 0 };
	std::atomic<bool> canStartThreadWork{ false };
	
	auto thr1 = std::thread(&AMTWStringUnsyncUpdateThread, std::ref(s), std::ref(threadsStarted), std::ref(canStartThreadWork));
	auto thr2 = std::thread(&AMTWStringUnsyncUpdateThread, std::ref(s), std::ref(threadsStarted), std::ref(canStartThreadWork));
	while (threadsStarted < 2);	
	amtWStringErrorsCount = 0;
	amt::SetCustomAssertHandler<0>(& AMTWStringUnsynchWriteTest_CustomAssertHandler);
	canStartThreadWork = true;
	thr1.join();
	thr2.join();
	amt::SetCustomAssertHandler<0>(nullptr);

	#if AMTL_MAIN_FEATURE_ON 
	EXPECT_NE(amtWStringErrorsCount.load(), 0);
	#else
	EXPECT_EQ(amtWStringErrorsCount.load(), 0);
	#endif
}

// ---------------------------------------------

TEST(__AMT_TEST__, AMTStringBasicTest)
{
	amt::string str("str");
	EXPECT_EQ(str.length(), 3);
	EXPECT_EQ(str[1], 't');
	str += str;
	EXPECT_EQ(str.size(), 6);
	EXPECT_EQ(str, "strstr");
	str.push_back(' ');
	EXPECT_EQ(str.size(), 7);
	EXPECT_EQ(str, "strstr ");
	str.pop_back();
	str.pop_back();
	EXPECT_EQ(str.size(), 5);
	EXPECT_EQ(str, "strst");
	str = str.substr(1, 2);
	EXPECT_EQ(str.size(), 2);
	EXPECT_EQ(str, "tr");

	amt::string str2(str);
	EXPECT_EQ(str2.size(), 2);
	EXPECT_EQ(str2, "tr");

	amt::string str3 = std::move(str2);
	EXPECT_EQ(str3.size(), 2);
	EXPECT_EQ(str3, "tr");

	amt::string str4(str3.begin(), str3.end());
	EXPECT_EQ(str4.size(), 2);
	EXPECT_EQ(str4, "tr");
	EXPECT_EQ(str3, str4);
	EXPECT_EQ(str3, str4.c_str());

	auto it = str4.begin();
	auto itEnd = str4.end();
	EXPECT_NE(it, itEnd);	

	amt::string strToCopyToVec("YetAnotherString");
	amt::vector<char> vecFromString(str.begin() + 3, str.begin() + 10);
	/*amt::string fromVector(&vecFromString[0], vecFromString.size());
	EXPECT_EQ(fromVector, "Another");*/
}

TEST(__AMT_TEST__, AMTStringIteratorValidityTest)
{
	bool assertionFailed = false;
	amt::SetThrowCustomAssertHandler<0>();
	amt::string str;

	auto cit = str.begin();
	if (cit == str.end()); //these are just the checks.. 
	EXPECT_EQ(cit, str.end()); //...that this code compiles

	auto it = str.begin();
	it = it;
	EXPECT_EQ(it, it);

	try {
		#if AMTL_CHECK_ITERATORS_VALIDITY_ON
		++it; // cannot increment on end
		#endif
	}
	catch (...)
	{
		assertionFailed = true;
	}
	EXPECT_EQ(assertionFailed, AMTL_CHECK_ITERATORS_VALIDITY_ON);

	assertionFailed = false;
	amt::string str2;
	try {
		#if AMTL_CHECK_ITERATORS_VALIDITY_ON
		if (it == str2.begin()) // cannot compare iterators of different instances
			str2.push_back(' ');
		#endif
	}
	catch (...)
	{
		assertionFailed = true;
	}
	EXPECT_EQ(assertionFailed, AMTL_CHECK_ITERATORS_VALIDITY_ON);

	assertionFailed = false;
	it = str2.end();
	str2.push_back(' '); // invalidates iterator
	try {
		#if AMTL_CHECK_ITERATORS_VALIDITY_ON
		++ it; // cannot use an invalidated operator
		#endif
	}
	catch (...)
	{
		assertionFailed = true;
	}

	EXPECT_EQ(assertionFailed, AMTL_CHECK_ITERATORS_VALIDITY_ON);

	str2 = "abc";
	it = str2.begin();
	EXPECT_EQ(*it, 'a');
	++it;
	EXPECT_EQ(*it, 'b');
	++it;
	EXPECT_EQ(*it, 'c');
	++it;
	EXPECT_EQ(it, str2.end());
}

std::atomic<size_t> amtStringErrorsCount{ 0 };

void AMTStringUnsynchWriteTest_CustomAssertHandler(bool isAssertionOk, const char* szFileName, long lLine, const char* szDesc)
{
	if (!isAssertionOk)
	{
		++amtStringErrorsCount;
	}
}

void AMTStringUnsyncUpdateThread(amt::string& s, std::atomic<size_t>& threadsStarted, std::atomic<bool>& canStartThreadWork)
{
	++threadsStarted;
	while (!canStartThreadWork);

	auto len = s.size();
	for (size_t i = 0 ; i < 65536 * 4 && amtStringErrorsCount == 0 ; ++ i)
	{
		for (size_t i = 0; i < len; ++i)
			s[i] = i; // unsynchronized write to a string				
	}
}

TEST(__AMT_TEST__, AMTStringUnsyncUpdate)
{
	amt::string s("abcdefghijklmnopqrstuvwxyz");
	std::atomic<size_t> threadsStarted{ 0 };
	std::atomic<bool> canStartThreadWork{ false };
	
	auto thr1 = std::thread(&AMTStringUnsyncUpdateThread, std::ref(s), std::ref(threadsStarted), std::ref(canStartThreadWork));
	auto thr2 = std::thread(&AMTStringUnsyncUpdateThread, std::ref(s), std::ref(threadsStarted), std::ref(canStartThreadWork));
	while (threadsStarted < 2);	
	amtStringErrorsCount = 0;
	amt::SetCustomAssertHandler<0>(& AMTStringUnsynchWriteTest_CustomAssertHandler);
	canStartThreadWork = true;
	thr1.join();
	thr2.join();
	amt::SetCustomAssertHandler<0>(nullptr);

	#if AMTL_MAIN_FEATURE_ON 
	EXPECT_NE(amtStringErrorsCount.load(), 0);
	#else
	EXPECT_EQ(amtStringErrorsCount.load(), 0);
	#endif
}

#ifdef __AMT_TEST_WITHOUT_GTEST__
int main()
{
	RUNTEST(__AMT_TEST__, BasicTest);
	RUNTEST(__AMT_TEST__, BasicPointerTest);
	RUNTEST(__AMT_TEST__, AMTPointerTypeTest);
	RUNTEST(__AMT_TEST__, BasicArithmeticsTest);
	RUNTEST(__AMT_TEST__, RemainingOperatorsTest);
	RUNTEST(__AMT_TEST__, LongLongTest);
	RUNTEST(__AMT_TEST__, LongLongOverflowTest);
	RUNTEST(__AMT_TEST__, LongLongAdditionTest);
	RUNTEST(__AMT_TEST__, LongLongSubtractionTest);
	RUNTEST(__AMT_TEST__, LongLongDivTest);
	RUNTEST(__AMT_TEST__, ScalarOperatorsStressTest);
	RUNTEST(__AMT_TEST__, BasicVectorTest);
	RUNTEST(__AMT_TEST__, BasicMapTest);
	RUNTEST(__AMT_TEST__, BasicSetTest);
	RUNTEST(__AMT_TEST__, LessBasicSetTest);
	RUNTEST(__AMT_TEST__, IntUnsynchWriteTest);
	RUNTEST(__AMT_TEST__, VectorSynchWriteTest);
	RUNTEST(__AMT_TEST__, VectorUnsynchWriteTest);
	RUNTEST(__AMT_TEST__, VectorInitializationTest);
	RUNTEST(__AMT_TEST__, MapUnsynchWriteTest);
	RUNTEST(__AMT_TEST__, MapInitializationTest);
	RUNTEST(__AMT_TEST__, SetInitializationTest);
	RUNTEST(__AMT_TEST__, SetUnsynchWriteTest);
	RUNTEST(__AMT_TEST__, SetCheckIteratorValidityTest);
	RUNTEST(__AMT_TEST__, SetCheckIteratorValidityTest_2);
	RUNTEST(__AMT_TEST__, SetCheckIteratorValidityTest_3);
	RUNTEST(__AMT_TEST__, SetCheckIteratorValidityTest_4);
	RUNTEST(__AMT_TEST__, SetIter_UnsynchUpdateTest);
	RUNTEST(__AMT_TEST__, MapCheckIteratorValidityTest);
	RUNTEST(__AMT_TEST__, MapCheckIteratorValidityTest_2);
	RUNTEST(__AMT_TEST__, MapCheckIteratorValidityTest_3);
	RUNTEST(__AMT_TEST__, MapCheckIteratorValidityTest_4);
	RUNTEST(__AMT_TEST__, MapIter_UnsynchUpdateTest);
	RUNTEST(__AMT_TEST__, TestObjectRawDataDebugChecker);
	RUNTEST(__AMT_TEST__, TestObjectRawDataDebugChecker_2);
	RUNTEST(__AMT_TEST__, TestObjectRawDataDebugChecker_AllOK);
	RUNTEST(__AMT_TEST__, CharNumericOverflowTest_AllOK);
	RUNTEST(__AMT_TEST__, CharNumericOverflowTest_Add);
	RUNTEST(__AMT_TEST__, CharNumericOverflowTest_Inc);
	RUNTEST(__AMT_TEST__, CharNumericOverflowTest_PostInc);
	RUNTEST(__AMT_TEST__, CharNumericOverflowTest_Subtract_1);
	RUNTEST(__AMT_TEST__, CharNumericOverflowTest_Subtract_2);
	RUNTEST(__AMT_TEST__, CharNumericOverflowTest_Subtract_3);
	RUNTEST(__AMT_TEST__, CharNumericOverflowTest_Subtract_4);
	RUNTEST(__AMT_TEST__, CharNumericOverflowTest_Dec);
	RUNTEST(__AMT_TEST__, CharNumericOverflowTest_PostDec);
	RUNTEST(__AMT_TEST__, CharNumericOverflowTest_Mul);
	RUNTEST(__AMT_TEST__, CharNumericOverflowTest_Div);
	RUNTEST(__AMT_TEST__, CharNumericOverflowTest_DivFloat);
	RUNTEST(__AMT_TEST__, CharNumericOverflowTest_DivZero);
	RUNTEST(__AMT_TEST__, UCharNumericOverflowTest_AllOK);
	RUNTEST(__AMT_TEST__, UCharNumericOverflowTest_Add);
	RUNTEST(__AMT_TEST__, UCharNumericOverflowTest_Inc);
	RUNTEST(__AMT_TEST__, UCharNumericOverflowTest_PostInc);
	RUNTEST(__AMT_TEST__, UCharNumericOverflowTest_Sub);
	RUNTEST(__AMT_TEST__, UCharNumericOverflowTest_Dec);
	RUNTEST(__AMT_TEST__, UCharNumericOverflowTest_PostDec);
	RUNTEST(__AMT_TEST__, UCharNumericOverflowTest_Mul_fine);
	RUNTEST(__AMT_TEST__, UCharNumericOverflowTest_Mul);
	RUNTEST(__AMT_TEST__, UCharNumericOverflowTest_MulFloat);
	RUNTEST(__AMT_TEST__, UCharNumericOverflowTest_MulNeg);
	RUNTEST(__AMT_TEST__, UCharNumericOverflowTest_Div);
	RUNTEST(__AMT_TEST__, UCharNumericOverflowTest_DivFloat);
	RUNTEST(__AMT_TEST__, UCharNumericOverflowTest_DivZero);
	RUNTEST(__AMT_TEST__, UCharRestFromDivision);
	RUNTEST(__AMT_TEST__, DoubleNumericOverflowTest_AllOK);
	RUNTEST(__AMT_TEST__, DoubleCorrectArithmeticsTest);
	RUNTEST(__AMT_TEST__, NumericLimitsTest);
	RUNTEST(__AMT_TEST__, EmplaceTest);
	RUNTEST(__AMT_TEST__, VectorOfAMTDoubleInitializationTest);
	RUNTEST(__AMT_TEST__, AMTVectorOfAMTDoubleInitializationTest);
	RUNTEST(__AMT_TEST__, AMTWStringBasicTest);
	RUNTEST(__AMT_TEST__, AMTWStringIteratorValidityTest);
	RUNTEST(__AMT_TEST__, AMTWStringUnsyncUpdate);
	RUNTEST(__AMT_TEST__, AMTStringBasicTest);
	RUNTEST(__AMT_TEST__, AMTStringIteratorValidityTest);
	RUNTEST(__AMT_TEST__, AMTStringUnsyncUpdate);

	return 1;
}
#endif
