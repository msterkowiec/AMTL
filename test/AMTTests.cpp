#include <gtest/gtest.h>

#include <thread>
#include <mutex>
#include <atomic>
#include "amt_vector.h"

TEST(AMTTest, BasicTest) {

	amt::vector<int> vec;
	EXPECT_EQ(vec.size(), 0);
	vec.emplace_back(10);
	EXPECT_EQ(vec.size(), 1);
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
		while (i >= size)
		{
			if (VectorSynchWriteTest_AssertionFailed)
				return;			
			i = size - 1;		
		}
		++ vec[i];
		size_t size = GetCurrentSize(vec);	
	}
	return;
}

TEST(AMTTest, VectorSynchWriteTest) {
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
		while (i >= vec.size())
		{
			if (VectorSynchWriteTest_AssertionFailed)
				return;
			i = vec.size() - 1;
		}
		++ vec[i];
	}
	return;
}

TEST(AMTTest, VectorUnsynchWriteTest) {
	amt::SetCustomAssertHandler<0>(&VectorUnsynchWriteTest_CustomAssertHandler);
	amt::vector<int> vec;
	std::thread thread1(&VectorUnsynchWriteTest_WriterThread, 0, std::ref(vec));
	std::thread thread2(&VectorUnsynchWriteTest_ReaderThread, 1, std::ref(vec));
	thread1.join();
	thread2.join();	
	EXPECT_EQ(VectorUnsynchWriteTest_AssertionFailed, true);
}
