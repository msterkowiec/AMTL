#include <gtest/gtest.h>

#include <thread>
#include <atomic>
#include "amt_vector.h"

TEST(AMTTest, BasicTest) {

	amt::vector<int> vec;
	EXPECT_EQ(vec.size(), 0);
	vec.emplace_back(10);
	EXPECT_EQ(vec.size(), 1);
}

bool VectorUnsynchWriteTestFuncExcCaught[2] = {false, false};
std::atomic<size_t> VectorUnsynchWriteTest_ThreadsComplete;

void VectorUnsynchWriteTestFunc(size_t threadNo, amt::vector<int>& vec)
{
	try{
		for (size_t i = 0 ; i < 32678 ; ++ i)
			vec.push_back(i);
	}
	catch(amt::AMTCassertException& e)
	{
		VectorUnsynchWriteTestFuncExcCaught[threadNo] = true;
	}
	++ VectorUnsynchWriteTest_ThreadsComplete;
	return;
}

TEST(AMTTest, VectorUnsynchWriteTest) {
	amt::SetThrowCustomAssertHandler<0>();
	amt::vector<int> vec;
	VectorUnsynchWriteTest_ThreadsComplete = 0;
	std::thread thread1(0, vec, &VectorUnsynchWriteTestFunc);
	std::thread thread2(1, vec, &VectorUnsynchWriteTestFunc);
	while (VectorUnsynchWriteTest_ThreadsComplete != 2)
		std::this_thread::yield();
	EXPECT_EQ(VectorUnsynchWriteTestFuncExcCaught[0], true);
	EXPECT_EQ(VectorUnsynchWriteTestFuncExcCaught[1], true);
}
