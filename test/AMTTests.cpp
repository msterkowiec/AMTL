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
// Test vector for unsynchronized access when writing. Expected assertion failure.

bool VectorUnsynchWriteTestFunc_AssertionFailed = false;

void VectorUnsynchWriteTest_CustomAssertHandler(bool a, const char* szFileName, long lLine, const char* szDesc)
{
	if (!a)
		if(strstr(szDesc, "m_nPendingWriteRequests == 0") != nullptr) // make sure this the assertion we expect		
			VectorUnsynchWriteTestFunc_AssertionFailed = true;
}

void VectorUnsynchWriteTest_WriterThread(size_t threadNo, amt::vector<int>& vec)
{
	for (size_t i = 0; i < 32678 && !VectorUnsynchWriteTestFunc_AssertionFailed; ++i)
		vec.push_back(i);
	return;
}
void VectorUnsynchWriteTest_ReaderThread(size_t threadNo, amt::vector<int>& vec)
{
	for (size_t i = 0; i < vec.size() && !VectorUnsynchWriteTestFunc_AssertionFailed; ++i)
		++ vec[i];
	++VectorUnsynchWriteTest_ThreadsComplete;
	return;
}

TEST(AMTTest, VectorUnsynchWriteTest) {
	amt::SetCustomAssertHandler<0>(&VectorUnsynchWriteTest_CustomAssertHandler);
	amt::vector<int> vec;
	std::thread thread1(&VectorUnsynchWriteTest_WriterThread, 0, std::ref(vec));
	std::thread thread2(&VectorUnsynchWriteTest_ReaderThread, 1, std::ref(vec));
	thread1.join();
	thread2.join();	
	EXPECT_EQ(VectorUnsynchWriteTestFunc_AssertionFailed, true);
}
