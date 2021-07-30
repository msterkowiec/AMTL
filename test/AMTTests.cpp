#include <gtest/gtest.h>

#include <thread>
#include <mutex>
#include <atomic>
#include <cstdlib>
#include <time.h> 
#include "amt_vector.h"
#include "amt_pod.h"
#ifdef _WIN32 
#include "amt_map.h"
#endif

TEST(AMTTest, BasicTest) {

	amt::vector<int> vec;
	EXPECT_EQ(vec.size(), 0);
	vec.emplace_back(10);
	EXPECT_EQ(vec.size(), 1);
}

// ----------------------------------------------------------------------
// Test unsynchronized access to integer

bool IntUnsynchWriteTest_AssertionFailed = false;

void IntUnsynchWriteTest_CustomAssertHandler(bool a, const char* szFileName, long lLine, const char* szDesc)
{
	if (!a)
		if(strstr(szDesc, "m_nPendingWriteRequests") != nullptr) // make sure this the assertion we expect		
			IntUnsynchWriteTest_AssertionFailed = true;
}

void IntUnsynchWriteTest_WriterThread(size_t threadNo, amt::int32_t& val)
{
	for (size_t i = 0; i < 32678 && !IntUnsynchWriteTest_AssertionFailed; ++i)
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
		if(strstr(szDesc, "m_nPendingWriteRequests == 0") != nullptr) // make sure this the assertion we expect		
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
		if(strstr(szDesc, "m_nPendingWriteRequests == 0") != nullptr) // make sure this the assertion we expect		
			VectorUnsynchWriteTest_AssertionFailed = true;
}

void VectorUnsynchWriteTest_WriterThread(size_t threadNo, amt::vector<int>& vec)
{
	for (size_t i = 0; i < 32678 && !VectorUnsynchWriteTest_AssertionFailed; ++i)
		vec.push_back(i);
	return;
}
void VectorUnsynchWriteTest_ReaderThread(size_t threadNo, amt::vector<int>& vec)
{
	for (size_t i = 0; i < 32678 && !VectorUnsynchWriteTest_AssertionFailed; ++i)
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

//#ifdef _WIN32  // temporarily commented out for Linux until fixed gcc-specific compilation issue: "explicit specialization in non-namespace scope"

bool MapUnsynchWriteTest_AssertionFailed = false;

void MapUnsynchWriteTest_CustomAssertHandler(bool a, const char* szFileName, long lLine, const char* szDesc)
{
	if (!a)
		if(strstr(szDesc, "m_nPendingWriteRequests") != nullptr) // make sure this the assertion we expect		
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

//#endif
