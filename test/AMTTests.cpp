#include <gtest/gtest.h>

#include <thread>
#include <mutex>
#include <atomic>
#include <cstdlib>
#include <time.h> 
#include "amt_vector.h"
#include "amt_pod.h"
#include "amt_map.h"
#include "amt_set.h"

TEST(AMTTest, BasicTest) {

	amt::vector<int> vec;
	EXPECT_EQ(vec.size(), 0);
	vec.emplace_back(10);
	EXPECT_EQ(vec.size(), 1);
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
		if(strstr(szDesc, "m_nPendingWriteRequests == 0") != nullptr) // make sure this is the assertion we expect		
			VectorUnsynchWriteTest_AssertionFailed = true;
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

// ----------------------------------------------------------------------
// Test set for unsynchronized access when writing. Expected assertion failure.

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
	auto it = set.begin();
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

// ----------------------------------------------------------------------------
// Tests of iterators of amt::map

TEST(AMTTest, MapCheckIteratorValidityTest) {
	bool assertionFailed = false;
	amt::SetThrowCustomAssertHandler<0>();
	amt::map<int, int> map;
	auto it = map.begin();
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

// =================================================================================================
// Group of test for numeric overflow
// ----------------------------------------------------------------------------
// Tests for char (amt::_char or amt::int8_t)

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

// ------------------------------------------------------
// Tests for unsigned char (amt::uint8_t)

TEST(AMTTest, UCharNumericOverflowTest_Add) {
	bool assertionFailed = false;
	amt::SetThrowCustomAssertHandler<0>();
	amt::uint8_t ch = 254;	
	try
	{
		ch += 2; // numeric overflow; unsigned char max is 255
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
	amt::uint8_t ch = 255;	
	try
	{
		++ch; // numeric overflow; unsigned char max is 255
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
	amt::uint8_t ch = 255;	
	try
	{
		ch++; // numeric overflow; unsigned char max is 255
	}
	catch(amt::AMTCassertException& e)
	{
		assertionFailed = true;
	}
	EXPECT_EQ(assertionFailed, true);
}
