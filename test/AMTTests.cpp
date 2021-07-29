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

bool VectorUnsynchWriteTestFunc_AssertionFailed = false;
std::atomic<size_t> VectorUnsynchWriteTest_ThreadsComplete;

void VectorUnsynchWriteTest_CustomAssertHandler(bool a, const char* szFileName, long lLine, const char* szDesc)
{
	if (!a)
		if(strstr(szDesc, "m_nPendingWriteRequests == 0") != nullptr) // make sure this the assertion we expect
		{
			VectorUnsynchWriteTestFunc_AssertionFailed = true;
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
}

void VectorUnsynchWriteTestFunc(size_t threadNo, amt::vector<int>& vec)
{
	for (size_t i = 0; i < 32678 && !VectorUnsynchWriteTestFunc_AssertionFailed; ++i)
			vec.push_back(i);
	++VectorUnsynchWriteTest_ThreadsComplete;
	return;
}


TEST(AMTTest, VectorUnsynchWriteTest) {
	amt::SetCustomAssertHandler<0>(&VectorUnsynchWriteTest_CustomAssertHandler);
	amt::vector<int> vec;
	VectorUnsynchWriteTest_ThreadsComplete = 0;
	std::thread thread1(&VectorUnsynchWriteTestFunc, 0, std::ref(vec));
	std::thread thread2(&VectorUnsynchWriteTestFunc, 1, std::ref(vec));
	while (VectorUnsynchWriteTest_ThreadsComplete != 2)
		std::this_thread::yield();
	 EXPECT_EQ(VectorUnsynchWriteTestFunc_AssertionFailed, true);
}
