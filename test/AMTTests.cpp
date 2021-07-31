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

void IntUnsynchWriteTest_WriterThread(size_t threadNo, amt::int32_t& val)
{
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
	std::thread thread1(&IntUnsynchWriteTest_WriterThread, 0, std::ref(val));
	std::thread thread2(&IntUnsynchWriteTest_WriterThread, 1, std::ref(val));
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

void VectorUnsynchWriteTest_WriterThread(size_t threadNo, amt::vector<int>& vec)
{
	for (size_t i = 0; i < 65536UL * 4 && !VectorUnsynchWriteTest_AssertionFailed; ++i)
		vec.push_back(i);
	return;
}
void VectorUnsynchWriteTest_ReaderThread(size_t threadNo, amt::vector<int>& vec)
{
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
	std::thread thread1(&VectorUnsynchWriteTest_WriterThread, 0, std::ref(vec));
	std::thread thread2(&VectorUnsynchWriteTest_ReaderThread, 1, std::ref(vec));
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

void MapUnsynchWriteTest_WriterThread(size_t threadNo, amt::map<int, int>& map)
{
	size_t iStart = threadNo ? 32678 : 0;
	size_t iEnd = threadNo ? 65536 : 32768;
	for (size_t i = iStart; i < iEnd && !MapUnsynchWriteTest_AssertionFailed; ++i)
		map[i] = i + threadNo;
}

TEST(AMTTest, MapUnsynchWriteTest) {
	srand (time(NULL));
	amt::SetCustomAssertHandler<0>(&MapUnsynchWriteTest_CustomAssertHandler);
	amt::map<int, int> _map;
	std::thread thread1(&MapUnsynchWriteTest_WriterThread, 0, std::ref(_map));
	std::thread thread2(&MapUnsynchWriteTest_WriterThread, 1, std::ref(_map));
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

void SetUnsynchWriteTest_WriterThread(size_t threadNo, amt::set<int>& set)
{
	size_t iStart = threadNo ? 32678 : 0;
	size_t iEnd = threadNo ? 65536 : 32768;
	for (size_t i = iStart; i < iEnd && !SetUnsynchWriteTest_AssertionFailed; ++i)
		set.insert(i);
}

TEST(AMTTest, SetUnsynchWriteTest) {
	srand (time(NULL));
	amt::SetCustomAssertHandler<0>(&SetUnsynchWriteTest_CustomAssertHandler);
	amt::set<int> set;
	std::thread thread1(&SetUnsynchWriteTest_WriterThread, 0, std::ref(set));
	std::thread thread2(&SetUnsynchWriteTest_WriterThread, 1, std::ref(set));
	thread1.join();
	thread2.join();	
	EXPECT_EQ(SetUnsynchWriteTest_AssertionFailed, true);
}

// =================================================================================================
// Group of test for invalid iterators
// ----------------------------------------------------------------------------

bool SetIteratorValidity_AssertionFailed = false;

void SetIteratorValidity_CustomAssertHandler(bool a, const char* szFileName, long lLine, const char* szDesc)
{
	if (!a)
		if(strstr(szDesc, "IsIteratorValid()") != nullptr || strstr(szDesc, "m_pSet") != nullptr || strstr(szDesc, "set") != nullptr) // make sure this is the assertion we expect		
			SetIteratorValidity_AssertionFailed = true;
}

TEST(AMTTest, SetCheckIteratorValidityTest) {
	SetIteratorValidity_AssertionFailed = false;
	amt::SetCustomAssertHandler<0>(& SetIteratorValidity_CustomAssertHandler);
	amt::set<int> set;
	auto it = set.begin();
	++ it; // cannot increment on end
	EXPECT_EQ(SetIteratorValidity_AssertionFailed, true);
}

TEST(AMTTest, SetCheckIteratorValidityTest_2) {
	SetIteratorValidity_AssertionFailed = false;
	amt::SetCustomAssertHandler<0>(& SetIteratorValidity_CustomAssertHandler);
	amt::set<int> set;
	auto it = set.begin();
	-- it; // cannot decrement on begin
	EXPECT_EQ(SetIteratorValidity_AssertionFailed, true);
}

TEST(AMTTest, SetCheckIteratorValidityTest_3) {
	SetIteratorValidity_AssertionFailed = false;
	amt::SetCustomAssertHandler<0>(& SetIteratorValidity_CustomAssertHandler);
	amt::set<int> set;
	auto it = set.begin();
	set.insert(1); // invalidates it
	bool isEnd = it == set.end();
	EXPECT_EQ(SetIteratorValidity_AssertionFailed, true);
}

TEST(AMTTest, SetCheckIteratorValidityTest_4) {
	SetIteratorValidity_AssertionFailed = false;
	amt::SetCustomAssertHandler<0>(& SetIteratorValidity_CustomAssertHandler);
	amt::set<int> set;
	amt::set<int> set2;
	auto it = set.begin();
	bool isEnd = it == set2.end(); // cannot use it of set vs end() of different object (set2)
	EXPECT_EQ(SetIteratorValidity_AssertionFailed, true);
}
