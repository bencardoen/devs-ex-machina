#include <gtest/gtest.h>
#include "tools/globallog.h"
#include "pools/pools.h"

LOG_INIT("out.txt")



int main(int argc, char** argv)
{
        n_pools::getMainThreadID();     // Make sure that, even if we never create controller, everyone can still check main'id.
	LOG_ARGV(argc, argv);
	::testing::InitGoogleTest(&argc, argv);
	//gtest intercepts exceptions, else we need try/catch to force stackunwind.
	return RUN_ALL_TESTS();
}
