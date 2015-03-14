#include <gtest/gtest.h>
#include "tools/globallog.h"

LOG_INIT("out.txt")

int main(int argc, char** argv)
{
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
