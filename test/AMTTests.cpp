// Make sure this line is commented out before committing to github
// #define __AMT_TEST_WITHOUT_GTEST__

#ifdef __AMT_TEST_WITHOUT_GTEST__
#define TEST(TestSuiteName, TestName) void TestSuiteName##TestName()
#define RUNTEST(TestSuiteName, TestName) TestSuiteName##TestName()
#define EXPECT_EQ(a,b) AMT_CASSERT((a)==(b))
#define EXPECT_NE(a,b) AMT_CASSERT((a)!=(b))
#define EXPECT_GE(a,b) AMT_CASSERT((a)>=(b))
#else
#include <gtest/gtest.h>
#endif

#include <thread>
#include <mutex>
#include <atomic>
#include <cstdlib>
#include <algorithm>
#include <time.h> 

// Override these macros for sake of tests before inclusion of AMTL headers:
#define __AMTL_ASSERTS_ARE_ON__
#define __AMT_CHECK_MULTITHREADED_ISSUES__ 1
#define __AMT_CHECK_ITERATORS_VALIDITY__ 1
#define __AMT_CHECK_SYNC_OF_ACCESS_TO_ITERATORS__ 1
#define __AMT_CHECK_NUMERIC_OVERFLOW__ 1
#define __AMT_LET_DESTRUCTORS_THROW__ 1
#define __AMT_DEBUG__ 1
#ifndef __AMT_TEST_WITHOUT_GTEST__
#define __AMTL_USE_STANDARD_ASSERT__ 1
#endif

#include "amt_vector.h"
#include "amt_pod.h"
#include "amt_map.h"
#include "amt_set.h"
#include "amt_rawdatadebugchecker.h"

template<typename U, typename V>
__AMT_CONSTEXPR__ bool AreNumericTypesEquivalent()
{
	typedef amt::UnwrappedType<U, amt::is_specialization<U, amt::AMTScalarType>::value> UType;
	typedef amt::UnwrappedType<V, amt::is_specialization<V, amt::AMTScalarType>::value> VType;

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
		++ AssertionFailedSilently;
}


TEST(AMTTest, BasicTest){

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
}

TEST(AMTTest, BasicArithmeticsTest){
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

TEST(AMTTest, LongLongTest){
	unsigned long long xll = 9;
	long long yll = 10;
	auto zll = (xll - yll) / 2;

	amt::uint64_t xxll = 9;
	amt::int64_t yyll = 10;
	amt::SetCustomAssertHandler<0>(&SilentCustomAssertHandler);
	auto counter = AssertionFailedSilently;
	auto zzll = (xxll - yyll) / 2;
	amt::SetCustomAssertHandler<0>(nullptr);
	EXPECT_EQ(AssertionFailedSilently, counter + 1);
	EXPECT_EQ(zll, zzll);

	yyll = 0;
	EXPECT_EQ(yyll + 1, 1);
}

TEST(AMTTest, LongLongOverflowTest){
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
	EXPECT_EQ(exceptionCaught, true);
}

TEST(AMTTest, LongLongAdditionTest) {

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
	EXPECT_EQ(exceptionCaught, true);
	EXPECT_EQ(res, ares);
	EXPECT_EQ((AreNumericTypesEquivalent<decltype(res), decltype(ares)>()), true);
	
}

TEST(AMTTest, LongLongSubtractionTest) {

	unsigned long long ull = 10;
	long long ll = -0xFFFFFFFFFFFFFFULL;
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
	EXPECT_EQ(exceptionCaught, true);
	EXPECT_EQ(res, ares);
	EXPECT_EQ((AreNumericTypesEquivalent<decltype(res), decltype(ares)>()), true);
	
}

TEST(AMTTest, LongLongDivTest) {

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

TEST(AMTTest, BasicVectorTest) {

	amt::vector<int> vec;
	EXPECT_EQ(vec.size(), 0);
	EXPECT_EQ(vec.capacity(), 0);
	vec.reserve(32);
	EXPECT_EQ(vec.size(), 0);
	EXPECT_GE(vec.capacity(), 32);
	vec.emplace_back(10);
	EXPECT_EQ(vec.size(), 1);
	vec = vec;
	EXPECT_EQ(vec.size(), 1);
}

TEST(AMTTest, BasicMapTest) {

	amt::map<int, int> map;
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

	amt::map<int, int>::const_iterator cit(it);
	EXPECT_EQ(cit->first, 0);
	EXPECT_EQ(cit->second, 3);
}

TEST(AMTTest, BasicSetTest) {

	amt::set<int> set;
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
}

// ======================================================================
//  Group of tests for unsynchronized access
// ----------------------------------------------------------------------
// Test unsynchronized access to integer

bool IntUnsynchWriteTest_AssertionFailed = false;

void IntUnsynchWriteTest_CustomAssertHandler(bool a, const char* szFileName, long lLine, const char* szDesc)
{
	if (!a)
		if(strstr(szDesc, "m_nPendingWriteRequests") != nullptr) // make sure this is the assertion we expect		
			IntUnsynchWriteTest_AssertionFailed = true;
}

void IntUnsynchWriteTest_WriterThread(size_t threadNo, amt::int32_t& val, std::atomic<bool>& canStartThread)
{
	while(!canStartThread); // make sure threads start at the same time
	for (size_t i = 0; i < 65536UL * 4 && !IntUnsynchWriteTest_AssertionFailed; ++i)
		if (threadNo)
			++ val;
		else
			-- val;
	return;
}

TEST(AMTTest, IntUnsynchWriteTest) {
	srand (time(NULL));
	amt::SetCustomAssertHandler<0>(&IntUnsynchWriteTest_CustomAssertHandler);
	amt::int32_t val;
	std::atomic<bool> canStartThread(false);
	std::thread thread1(&IntUnsynchWriteTest_WriterThread, 0, std::ref(val), std::ref(canStartThread));
	std::thread thread2(&IntUnsynchWriteTest_WriterThread, 1, std::ref(val), std::ref(canStartThread));
	canStartThread = true;
	thread1.join();
	thread2.join();	
	EXPECT_EQ(IntUnsynchWriteTest_AssertionFailed, true);
}

// ----------------------------------------------------------------------
// Test vector for synchronized access when writing. Expected no assertion failure.

bool VectorSynchWriteTest_AssertionFailed = false;
std::recursive_mutex mtxVectorSynchWriteTest;

void VectorSynchWriteTest_CustomAssertHandler(bool a, const char* szFileName, long lLine, const char* szDesc)
{
	if (!a)
		if(strstr(szDesc, "m_nPendingWriteRequests == 0") != nullptr) // make sure this is the assertion we expect		
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
			size_t idx = rand () % size;
			++ vec[idx];
		}
		size = GetCurrentSize(vec);	
	}
	return;
}

TEST(AMTTest, VectorSynchWriteTest) {
	srand (time(NULL));
	amt::SetCustomAssertHandler<0>(&VectorSynchWriteTest_CustomAssertHandler);
	amt::vector<int> vec;
	std::thread thread1(&VectorSynchWriteTest_WriterThread, 0, std::ref(vec));
	std::thread thread2(&VectorSynchWriteTest_ReaderThread, 1, std::ref(vec));
	thread1.join();
	thread2.join();	
	EXPECT_EQ(VectorSynchWriteTest_AssertionFailed, false);
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
	while(!canStartThread); // make sure threads start at the same time
	for (size_t i = 0; i < 65536UL * 4 && !VectorUnsynchWriteTest_AssertionFailed; ++i)
		vec.push_back(i);
	return;
}
void VectorUnsynchWriteTest_ReaderThread(size_t threadNo, amt::vector<int>& vec, std::atomic<bool>& canStartThread)
{
	while(!canStartThread); // make sure threads start at the same time
	for (size_t i = 0; i < 65536UL * 4 && !VectorUnsynchWriteTest_AssertionFailed; ++i)
	{
		if (vec.size())
		{
			size_t idx = rand () % vec.size();
			++ vec[idx];
		}
	}
	return;
}

TEST(AMTTest, VectorUnsynchWriteTest) {
	srand (time(NULL));
	amt::SetCustomAssertHandler<0>(&VectorUnsynchWriteTest_CustomAssertHandler);
	amt::vector<int> vec;
	std::atomic<bool> canStartThread(false);
	std::thread thread1(&VectorUnsynchWriteTest_WriterThread, 0, std::ref(vec), std::ref(canStartThread));
	std::thread thread2(&VectorUnsynchWriteTest_ReaderThread, 1, std::ref(vec), std::ref(canStartThread));
	canStartThread = true;
	thread1.join();
	thread2.join();	
	EXPECT_EQ(VectorUnsynchWriteTest_AssertionFailed, true);
}

struct TestPodStruct
{
	int i;
	double db;
};

TEST(AMTTest, VectorInitializationTest) {
	
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

void MapUnsynchWriteTest_CustomAssertHandler(bool a, const char* szFileName, long lLine, const char* szDesc)
{
	if (!a)
		if(strstr(szDesc, "m_nPendingWriteRequests") != nullptr) // make sure this is the assertion we expect		
			MapUnsynchWriteTest_AssertionFailed = true;
}

void MapUnsynchWriteTest_WriterThread(size_t threadNo, amt::map<int, int>& map, std::atomic<bool>& canStartThread)
{
	while(!canStartThread); // make sure threads start at the same time
	size_t iStart = threadNo ? 32678 : 0;
	size_t iEnd = threadNo ? 65536 : 32768;
	for (size_t i = iStart; i < iEnd && !MapUnsynchWriteTest_AssertionFailed; ++i)
		map[i] = i + threadNo;
}

TEST(AMTTest, MapUnsynchWriteTest) {
	srand (time(NULL));
	amt::SetCustomAssertHandler<0>(&MapUnsynchWriteTest_CustomAssertHandler);
	amt::map<int, int> _map;
	std::atomic<bool> canStartThread(false);
	std::thread thread1(&MapUnsynchWriteTest_WriterThread, 0, std::ref(_map), std::ref(canStartThread));
	std::thread thread2(&MapUnsynchWriteTest_WriterThread, 1, std::ref(_map), std::ref(canStartThread));
	canStartThread = true;
	thread1.join();
	thread2.join();	
	EXPECT_EQ(MapUnsynchWriteTest_AssertionFailed, true);
}

TEST(AMTTest, MapInitializationTest){
	amt::map<int, std::string> map{ { 1, "1" }, { 2, "2" } };
	EXPECT_EQ(map.size(), 2);
	EXPECT_EQ(map[1], "1");
	EXPECT_EQ((std::is_same<decltype(map)::mapped_type, std::string>::value), true);
	
	amt::map<int, std::string> map2(map.begin(), map.end());
	EXPECT_EQ(map2[2], "2");
	EXPECT_EQ(map2.size(), 2);
}

// ----------------------------------------------------------------------
// Test set for unsynchronized access when writing. Expected assertion failure.

TEST(AMTTest, SetInitializationTest)
{
	amt::set<std::string> set{ "1" , "2", "22" };
	EXPECT_EQ(set.size(), 3);
	EXPECT_EQ(*set.begin(), "1");
	EXPECT_EQ(*set.rbegin(), "22");
	EXPECT_EQ((std::is_same<decltype(set)::value_type, std::string>::value), true);

	amt::set<std::string> set2(set.begin(), set.end());
	EXPECT_EQ(set2.size(), 3);
}

bool SetUnsynchWriteTest_AssertionFailed = false;

void SetUnsynchWriteTest_CustomAssertHandler(bool a, const char* szFileName, long lLine, const char* szDesc)
{
	if (!a)
		if(strstr(szDesc, "m_nPendingWriteRequests") != nullptr) // make sure this is the assertion we expect		
			SetUnsynchWriteTest_AssertionFailed = true;
}

void SetUnsynchWriteTest_WriterThread(size_t threadNo, amt::set<int>& set, std::atomic<bool>& canStartThread)
{
	while(!canStartThread); // make sure threads start at the same time
	size_t iStart = threadNo ? 32678 : 0;
	size_t iEnd = threadNo ? 65536 : 32768;
	for (size_t i = iStart; i < iEnd && !SetUnsynchWriteTest_AssertionFailed; ++i)
		set.insert(i);
}

TEST(AMTTest, SetUnsynchWriteTest) {
	srand (time(NULL));
	amt::SetCustomAssertHandler<0>(&SetUnsynchWriteTest_CustomAssertHandler);
	amt::set<int> set;
	std::atomic<bool> canStartThread(false);
	std::thread thread1(&SetUnsynchWriteTest_WriterThread, 0, std::ref(set), std::ref(canStartThread));
	std::thread thread2(&SetUnsynchWriteTest_WriterThread, 1, std::ref(set), std::ref(canStartThread));
	canStartThread = true;
	thread1.join();
	thread2.join();	
	EXPECT_EQ(SetUnsynchWriteTest_AssertionFailed, true);
}

// =================================================================================================
// Group of test for invalid iterators
// ----------------------------------------------------------------------------
// Tests of iterators of amt::set

TEST(AMTTest, SetCheckIteratorValidityTest) {
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
		++ it; // cannot increment on end
	}
	catch(...)
	{
		assertionFailed = true;
	}
	EXPECT_EQ(assertionFailed, true);
}

TEST(AMTTest, SetCheckIteratorValidityTest_2) {
	bool assertionFailed = false;
	amt::SetThrowCustomAssertHandler<0>();
	amt::set<int> set;
	auto it = set.begin();
	try
	{
		-- it; // cannot decrement on begin
	}
	catch(...)
	{
		assertionFailed = true;
	}
	EXPECT_EQ(assertionFailed, true);
}

TEST(AMTTest, SetCheckIteratorValidityTest_3) {
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
	catch(...)
	{
		assertionFailed = true;
	}
	
	EXPECT_EQ(assertionFailed, true);
}

TEST(AMTTest, SetCheckIteratorValidityTest_4) {
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
	catch(...)
	{
		assertionFailed = true;
	}
	EXPECT_EQ(assertionFailed, true);
}

// ---------------------------------------------------------------------
// Test unsynchronized update of an iterator in different threads

bool SetIter_UnsynchUpdateTest_AssertionFailed = false;

void SetIter_UnsynchUpdateTest_CustomAssertHandler(bool a, const char* szFileName, long lLine, const char* szDesc)
{
	if (!a)
		if (strstr(szDesc, "m_nPendingWriteRequests") != nullptr) // make sure this is the assertion we expect
			SetIter_UnsynchUpdateTest_AssertionFailed = true;
}

void SetIter_UnsynchUpdateTest_WriterThread(size_t threadNo, amt::set<int>& set, amt::set<int>::iterator& it, std::atomic<bool>& canStartThread)
{
	while (!canStartThread); // make sure threads start at the same time			

	try
	{
		if (threadNo)
		{
			while (it != set.end())
				++it;
		}
		else
			while (it != set.begin())
				--it;
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

TEST(AMTTest, SetIter_UnsynchUpdateTest) {
	amt::SetCustomAssertHandler<0>(&SetIter_UnsynchUpdateTest_CustomAssertHandler);
	amt::set<int> set;
	for (int i = 0; i < 65536; ++i)
		set.insert(i);
	auto it = set.find(32768);
	std::atomic<bool> canStartThread(false);
	std::thread thread1(&SetIter_UnsynchUpdateTest_WriterThread, 0, std::ref(set), std::ref(it), std::ref(canStartThread));
	std::thread thread2(&SetIter_UnsynchUpdateTest_WriterThread, 1, std::ref(set), std::ref(it), std::ref(canStartThread));
	canStartThread = true;

	thread1.join();
	thread2.join();
	EXPECT_EQ(SetIter_UnsynchUpdateTest_AssertionFailed, true);
}

// ----------------------------------------------------------------------------
// Tests of iterators of amt::map

TEST(AMTTest, MapCheckIteratorValidityTest) {
	bool assertionFailed = false;
	amt::SetThrowCustomAssertHandler<0>();

	amt::map<int, int> map;	
	amt::map<int, int>::const_iterator cit = map.begin();
	if (cit == map.end()); //these are just the checks.. 
	EXPECT_EQ(cit, map.end()); //...that this code compiles	

	auto it = map.begin();
	it = it;
	EXPECT_EQ(it, it);
	try{
		++ it; // cannot increment on end
	}
	catch(...)
	{
		assertionFailed = true;
	}
	EXPECT_EQ(assertionFailed, true);
}

TEST(AMTTest, MapCheckIteratorValidityTest_2) {
	bool assertionFailed = false;
	amt::SetThrowCustomAssertHandler<0>();
	amt::map<int, int> map;
	auto it = map.begin();
	try
	{
		-- it; // cannot decrement on begin
	}
	catch(...)
	{
		assertionFailed = true;
	}
	EXPECT_EQ(assertionFailed, true);
}

TEST(AMTTest, MapCheckIteratorValidityTest_3) {
	bool assertionFailed = false;
	amt::SetThrowCustomAssertHandler<0>();
	amt::map<int, int> map;
	auto it = map.begin();
	map[1] = 1; // invalidates it
	bool isEnd = false;
	try
	{
		isEnd = it == map.end();
	}
	catch(...)
	{
		assertionFailed = true;
	}
	
	EXPECT_EQ(assertionFailed, true);
}

TEST(AMTTest, MapCheckIteratorValidityTest_4) {
	bool assertionFailed = false;
	amt::SetThrowCustomAssertHandler<0>();
	amt::map<int, int> map;
	amt::map<int, int> map2;
	auto it = map.begin();
	bool isEnd = false;
	try
	{
		isEnd = it == map2.end(); // cannot use it of set vs end() of different object (set2)
	}
	catch(...)
	{
		assertionFailed = true;
	}
	EXPECT_EQ(assertionFailed, true);
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

void MapIter_UnsynchUpdateTest_WriterThread(size_t threadNo, amt::map<int, int>& map, amt::map<int, int>::iterator& it, std::atomic<bool>& canStartThread)
{
	while (!canStartThread); // make sure threads start at the same time			

	try
	{
		if (threadNo)
		{
			while (it != map.end())
				++it;
		}
		else
			while (it != map.begin())
				--it;
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

TEST(AMTTest, MapIter_UnsynchUpdateTest) {
	amt::SetCustomAssertHandler<0>(&MapIter_UnsynchUpdateTest_CustomAssertHandler);
	amt::map<int, int> map;
	for (int i = 0; i < 65536; ++i)
		map[i] = i;
	auto it = map.find(32768);
	std::atomic<bool> canStartThread(false);
	std::thread thread1(&MapIter_UnsynchUpdateTest_WriterThread, 0, std::ref(map), std::ref(it), std::ref(canStartThread));
	std::thread thread2(&MapIter_UnsynchUpdateTest_WriterThread, 1, std::ref(map), std::ref(it), std::ref(canStartThread));
	canStartThread = true;

	thread1.join();
	thread2.join();
	EXPECT_EQ(MapIter_UnsynchUpdateTest_AssertionFailed, true);
}

// ===================================================================================================================
// Group of tests for TObjectRawDataDebugChecker - a class that asserts that some data stays intact/not overwritten meanwhile
// -----------------------------------------------------------
	
TEST(AMTTest, TestObjectRawDataDebugChecker) {
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
	EXPECT_EQ(assertionFailed, true);
}

TEST(AMTTest, TestObjectRawDataDebugChecker_2) {
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
	EXPECT_EQ(assertionFailed, true);
}

TEST(AMTTest, TestObjectRawDataDebugChecker_AllOK) {
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

TEST(AMTTest, CharNumericOverflowTest_AllOK) {
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
		++ ch;
		ch++;
		ch = ch * 1.5;
		ch *= 1.7;
		
		EXPECT_EQ((std::uint8_t) ch, 5);
		ch = 6 % ch;
	}
	catch(amt::AMTCassertException& e)
	{
		assertionFailed = true;
	}
	EXPECT_EQ(assertionFailed, false);
}

TEST(AMTTest, CharNumericOverflowTest_Add) {
	bool assertionFailed = false;
	amt::SetThrowCustomAssertHandler<0>();
	amt::int8_t ch = 100;	
	try
	{
		ch += 28; // overflow; char max is 127
	}
	catch(amt::AMTCassertException& e)
	{
		assertionFailed = true;
	}
	EXPECT_EQ(assertionFailed, true);
}

TEST(AMTTest, CharNumericOverflowTest_Inc) {
	bool assertionFailed = false;
	amt::SetThrowCustomAssertHandler<0>();
	amt::int8_t ch = 127;	
	try
	{
		++ch; // overflow; char max is 127
	}
	catch(amt::AMTCassertException& e)
	{
		assertionFailed = true;
	}
	EXPECT_EQ(assertionFailed, true);
}

TEST(AMTTest, CharNumericOverflowTest_PostInc) {
	bool assertionFailed = false;
	amt::SetThrowCustomAssertHandler<0>();
	amt::int8_t ch = 127;	
	try
	{
		ch++; // overflow; char max is 127
	}
	catch(amt::AMTCassertException& e)
	{
		assertionFailed = true;
	}
	EXPECT_EQ(assertionFailed, true);
}

TEST(AMTTest, CharNumericOverflowTest_Subtract_1) {
	bool assertionFailed = false;
	amt::SetThrowCustomAssertHandler<0>();
	amt::int8_t ch = -100;	
	try
	{
		ch -= 28; // no overflow; char min is -128
	}
	catch(amt::AMTCassertException& e)
	{
		assertionFailed = true;
	}
	EXPECT_EQ(assertionFailed, false);
}

TEST(AMTTest, CharNumericOverflowTest_Subtract_2) {
	bool assertionFailed = false;
	amt::SetThrowCustomAssertHandler<0>();
	amt::int8_t ch = -100;	
	try
	{
		ch -= 29; // overflow; char min is -128
	}
	catch(amt::AMTCassertException& e)
	{
		assertionFailed = true;
	}
	EXPECT_EQ(assertionFailed, true);
}

TEST(AMTTest, CharNumericOverflowTest_Dec) {
	bool assertionFailed = false;
	amt::SetThrowCustomAssertHandler<0>();
	amt::int8_t ch = -128;	
	try
	{
		--ch; // overflow; char min is -128
	}
	catch(amt::AMTCassertException& e)
	{
		assertionFailed = true;
	}
	EXPECT_EQ(assertionFailed, true);
}

TEST(AMTTest, CharNumericOverflowTest_PostDec) {
	bool assertionFailed = false;
	amt::SetThrowCustomAssertHandler<0>();
	amt::int8_t ch = -128;	
	try
	{
		ch--; // overflow; char min is -128
	}
	catch(amt::AMTCassertException& e)
	{
		assertionFailed = true;
	}
	EXPECT_EQ(assertionFailed, true);
}

TEST(AMTTest, CharNumericOverflowTest_Mul) {
	bool assertionFailed = false;
	amt::SetThrowCustomAssertHandler<0>();
	amt::int8_t ch = 50;	
	try
	{
		ch *= 3; // overflow
	}
	catch(amt::AMTCassertException& e)
	{
		assertionFailed = true;
	}
	EXPECT_EQ(assertionFailed, true);
}

TEST(AMTTest, CharNumericOverflowTest_Div) {
	bool assertionFailed = false;
	amt::SetThrowCustomAssertHandler<0>();
	amt::int8_t ch = -128;	
	try
	{
		ch /= -1; // overflow; char max is 127
	}
	catch(amt::AMTCassertException& e)
	{
		assertionFailed = true;
	}
	EXPECT_EQ(assertionFailed, true);
}

TEST(AMTTest, CharNumericOverflowTest_DivFloat) {
	bool assertionFailed = false;
	amt::SetThrowCustomAssertHandler<0>();
	amt::int8_t ch = 1;	
	try
	{
		ch /= -0.005; // overflow; char min is -128
	}
	catch(amt::AMTCassertException& e)
	{
		assertionFailed = true;
	}
	EXPECT_EQ(assertionFailed, true);
}

TEST(AMTTest, CharNumericOverflowTest_DivZero) {
	bool assertionFailed = false;
	amt::SetThrowCustomAssertHandler<0>();
	amt::int8_t ch = 1;	
	try
	{
		ch /= 0; 
	}
	catch(amt::AMTCassertException& e)
	{
		assertionFailed = true;
	}
	EXPECT_EQ(assertionFailed, true);
}

// ------------------------------------------------------
// Tests for unsigned char (amt::uint8_t)

TEST(AMTTest, UCharNumericOverflowTest_AllOK) {
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
		++ uch;
		uch++;
		uch = uch * 1.5;
		uch *= 1.7;
		
		EXPECT_EQ((std::uint8_t) uch, 5);
	}
	catch(amt::AMTCassertException& e)
	{
		assertionFailed = true;
	}
	EXPECT_EQ(assertionFailed, false);
}

TEST(AMTTest, UCharNumericOverflowTest_Add) {
	bool assertionFailed = false;
	amt::SetThrowCustomAssertHandler<0>();
	amt::uint8_t uch = 254;	
	try
	{
		uch += 2; // numeric overflow; unsigned char max is 255
	}
	catch(amt::AMTCassertException& e)
	{
		assertionFailed = true;
	}
	EXPECT_EQ(assertionFailed, true);
}

TEST(AMTTest, UCharNumericOverflowTest_Inc) {
	bool assertionFailed = false;
	amt::SetThrowCustomAssertHandler<0>();
	amt::uint8_t uch = 255;	
	try
	{
		++uch; // numeric overflow; unsigned char max is 255
	}
	catch(amt::AMTCassertException& e)
	{
		assertionFailed = true;
	}
	EXPECT_EQ(assertionFailed, true);
}

TEST(AMTTest, UCharNumericOverflowTest_PostInc) {
	bool assertionFailed = false;
	amt::SetThrowCustomAssertHandler<0>();
	amt::uint8_t uch = 255;	
	try
	{
		uch++; // numeric overflow; unsigned char max is 255
	}
	catch(amt::AMTCassertException& e)
	{
		assertionFailed = true;
	}
	EXPECT_EQ(assertionFailed, true);
}

TEST(AMTTest, UCharNumericOverflowTest_Sub) {
	bool assertionFailed = false;
	amt::SetThrowCustomAssertHandler<0>();
	amt::uint8_t uch = 5;	
	try
	{
		uch -= 10; // numeric overflow; unsigned char min is 0
	}
	catch(amt::AMTCassertException& e)
	{
		assertionFailed = true;
	}
	EXPECT_EQ(assertionFailed, true);
}

TEST(AMTTest, UCharNumericOverflowTest_Dec) {
	bool assertionFailed = false;
	amt::SetThrowCustomAssertHandler<0>();
	amt::uint8_t uch = 0;	
	try
	{
		--uch; // numeric overflow; unsigned char min is 0
	}
	catch(amt::AMTCassertException& e)
	{
		assertionFailed = true;
	}
	EXPECT_EQ(assertionFailed, true);
}

TEST(AMTTest, UCharNumericOverflowTest_PostDec) {
	bool assertionFailed = false;
	amt::SetThrowCustomAssertHandler<0>();
	amt::uint8_t uch = 0;	
	try
	{
		uch--; // numeric overflow; unsigned char min is 0
	}
	catch(amt::AMTCassertException& e)
	{
		assertionFailed = true;
	}
	EXPECT_EQ(assertionFailed, true);
}

TEST(AMTTest, UCharNumericOverflowTest_Mul_fine) {
	bool assertionFailed = false;
	amt::SetThrowCustomAssertHandler<0>();
	amt::uint8_t uch = 51;	
	try
	{
		uch *= 5; // ok, unsigned char max is 255
	}
	catch(amt::AMTCassertException& e)
	{
		assertionFailed = true;
	}
	EXPECT_EQ(assertionFailed, false);
}

TEST(AMTTest, UCharNumericOverflowTest_Mul) {
	bool assertionFailed = false;
	amt::SetThrowCustomAssertHandler<0>();
	amt::uint8_t uch = 16;	
	try
	{
		uch *= 16; // assertion failure, unsigned char max is 255
	}
	catch(amt::AMTCassertException& e)
	{
		assertionFailed = true;
	}
	EXPECT_EQ(assertionFailed, true);
}

TEST(AMTTest, UCharNumericOverflowTest_MulFloat) {
	bool assertionFailed = false;
	amt::SetThrowCustomAssertHandler<0>();
	amt::uint8_t uch = 100;	
	try
	{
		uch *= 2.6; // assertion failure, unsigned char max is 255
	}
	catch(amt::AMTCassertException& e)
	{
		assertionFailed = true;
	}
	EXPECT_EQ(assertionFailed, true);
}

TEST(AMTTest, UCharNumericOverflowTest_MulNeg) {
	bool assertionFailed = false;
	amt::SetThrowCustomAssertHandler<0>();
	amt::uint8_t uch = 16;	
	try
	{
		uch *= -1; // assertion failure
	}
	catch(amt::AMTCassertException& e)
	{
		assertionFailed = true;
	}
	EXPECT_EQ(assertionFailed, true);
}

TEST(AMTTest, UCharNumericOverflowTest_Div) {
	bool assertionFailed = false;
	amt::SetThrowCustomAssertHandler<0>();
	amt::uint8_t uch = 10;	
	try
	{
		uch /= -1; // assertion failure
	}
	catch(amt::AMTCassertException& e)
	{
		assertionFailed = true;
	}
	EXPECT_EQ(assertionFailed, true);
	assertionFailed = false; // reset this flag
	try
	{
		uch = uch / -1;
	}
	catch (amt::AMTCassertException& e)
	{
		assertionFailed = true;
	}
	EXPECT_EQ(assertionFailed, true);
}

TEST(AMTTest, UCharNumericOverflowTest_DivFloat) {
	bool assertionFailed = false;
	amt::SetThrowCustomAssertHandler<0>();
	amt::uint8_t uch = 100;	
	try
	{
		uch /= 0.3; // assertion failure
	}
	catch(amt::AMTCassertException& e)
	{
		assertionFailed = true;
	}
	EXPECT_EQ(assertionFailed, true);
}

TEST(AMTTest, UCharNumericOverflowTest_DivZero) {
	bool assertionFailed = false;
	amt::SetThrowCustomAssertHandler<0>();
	amt::uint8_t uch = 10;	
	try
	{
		uch /= 0; // assertion failure
	}
	catch(amt::AMTCassertException& e)
	{
		assertionFailed = true;
	}
	EXPECT_EQ(assertionFailed, true);
}

TEST(AMTTest, UCharRestFromDivision) {
	bool assertionFailed = false;
	amt::SetThrowCustomAssertHandler<0>();
	amt::uint8_t uch = 10;
	try
	{
		uch %= 4;
		EXPECT_EQ(uch, 2);
		uch %= 0;
	}
	catch (amt::AMTCassertException& e)
	{
		assertionFailed = true;
	}
	EXPECT_EQ(assertionFailed, true);
	assertionFailed = false; // reset this flag
	try
	{
		uch = uch % 0;
	}
	catch (amt::AMTCassertException& e)
	{
		assertionFailed = true;
	}
	EXPECT_EQ(assertionFailed, true);

}

// ------------------------------------------------------
// Tests for double (amt::_double)

TEST(AMTTest, DoubleNumericOverflowTest_AllOK) {
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

TEST(AMTTest, DoubleCorrectArithmeticsTest)
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
	EXPECT_EQ(assertionFailed, true);

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
	EXPECT_EQ(assertionFailed, true);
}

TEST(AMTTest, NumericLimitsTest)
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

TEST(AMTTest, EmplaceTest)
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

	amt::vector<Struct> vec;
	vec.emplace_back(std::vector<int>{ 1 });
	EXPECT_EQ(vec.size(), 1);
	EXPECT_EQ(vec[0].size(), 1);

	auto it = vec.emplace(vec.begin(), std::vector<int>{1, 2});
	EXPECT_NE(it, vec.end());
	EXPECT_EQ(it->size(), 2);

	amt::map<int, Struct> map;
	map.emplace(1, std::vector<int>{1, 2, 3});
	auto mit = map.find(1);
	EXPECT_EQ(mit->second.size(), 3);

	amt::set<Struct> set;
	set.emplace(std::vector<int>{1, 2, 3, 4});
	auto sit = set.begin();
	EXPECT_EQ(sit->size(), 4);

}

#ifdef __AMT_TEST_WITHOUT_GTEST__
int main()
{	
	RUNTEST(AMTTest, BasicTest);
	RUNTEST(AMTTest, BasicArithmeticsTest);
	RUNTEST(AMTTest, LongLongTest);
	RUNTEST(AMTTest, LongLongOverflowTest);
	RUNTEST(AMTTest, LongLongAdditionTest);
	RUNTEST(AMTTest, LongLongSubtractionTest);
	RUNTEST(AMTTest, LongLongDivTest);
	RUNTEST(AMTTest, BasicVectorTest);
	RUNTEST(AMTTest, BasicMapTest);
	RUNTEST(AMTTest, BasicSetTest);
	RUNTEST(AMTTest, IntUnsynchWriteTest);
	RUNTEST(AMTTest, VectorSynchWriteTest);
	RUNTEST(AMTTest, VectorUnsynchWriteTest);
	RUNTEST(AMTTest, VectorInitializationTest);
	RUNTEST(AMTTest, MapUnsynchWriteTest);
	RUNTEST(AMTTest, MapInitializationTest);
	RUNTEST(AMTTest, SetInitializationTest);
	RUNTEST(AMTTest, SetUnsynchWriteTest);
	RUNTEST(AMTTest, SetCheckIteratorValidityTest);
	RUNTEST(AMTTest, SetCheckIteratorValidityTest_2);
	RUNTEST(AMTTest, SetCheckIteratorValidityTest_3); 
	RUNTEST(AMTTest, SetCheckIteratorValidityTest_4);
	RUNTEST(AMTTest, SetIter_UnsynchUpdateTest);
	RUNTEST(AMTTest, MapCheckIteratorValidityTest);
	RUNTEST(AMTTest, MapCheckIteratorValidityTest_2);
	RUNTEST(AMTTest, MapCheckIteratorValidityTest_3);
	RUNTEST(AMTTest, MapCheckIteratorValidityTest_4);
	RUNTEST(AMTTest, MapIter_UnsynchUpdateTest);
	RUNTEST(AMTTest, TestObjectRawDataDebugChecker);
	RUNTEST(AMTTest, TestObjectRawDataDebugChecker_2);
	RUNTEST(AMTTest, TestObjectRawDataDebugChecker_AllOK);
	RUNTEST(AMTTest, CharNumericOverflowTest_AllOK);
	RUNTEST(AMTTest, CharNumericOverflowTest_Add);
	RUNTEST(AMTTest, CharNumericOverflowTest_Inc);
	RUNTEST(AMTTest, CharNumericOverflowTest_PostInc);
	RUNTEST(AMTTest, CharNumericOverflowTest_Subtract_1);
	RUNTEST(AMTTest, CharNumericOverflowTest_Subtract_2);
	RUNTEST(AMTTest, CharNumericOverflowTest_Dec);
	RUNTEST(AMTTest, CharNumericOverflowTest_PostDec);
	RUNTEST(AMTTest, CharNumericOverflowTest_Mul);
	RUNTEST(AMTTest, CharNumericOverflowTest_Div);
	RUNTEST(AMTTest, CharNumericOverflowTest_DivFloat);
	RUNTEST(AMTTest, CharNumericOverflowTest_DivZero);
	RUNTEST(AMTTest, UCharNumericOverflowTest_AllOK);
	RUNTEST(AMTTest, UCharNumericOverflowTest_Add);
	RUNTEST(AMTTest, UCharNumericOverflowTest_Inc);
	RUNTEST(AMTTest, UCharNumericOverflowTest_PostInc);
	RUNTEST(AMTTest, UCharNumericOverflowTest_Sub);
	RUNTEST(AMTTest, UCharNumericOverflowTest_Dec);
	RUNTEST(AMTTest, UCharNumericOverflowTest_PostDec);
	RUNTEST(AMTTest, UCharNumericOverflowTest_Mul_fine);
	RUNTEST(AMTTest, UCharNumericOverflowTest_Mul);
	RUNTEST(AMTTest, UCharNumericOverflowTest_MulFloat);
	RUNTEST(AMTTest, UCharNumericOverflowTest_MulNeg);
	RUNTEST(AMTTest, UCharNumericOverflowTest_Div);
	RUNTEST(AMTTest, UCharNumericOverflowTest_DivFloat);
	RUNTEST(AMTTest, UCharNumericOverflowTest_DivZero);
	RUNTEST(AMTTest, UCharRestFromDivision);
	RUNTEST(AMTTest, DoubleNumericOverflowTest_AllOK);
	RUNTEST(AMTTest, DoubleCorrectArithmeticsTest);
	RUNTEST(AMTTest, NumericLimitsTest);
	RUNTEST(AMTTest, EmplaceTest);

	return 1;
}
#endif
