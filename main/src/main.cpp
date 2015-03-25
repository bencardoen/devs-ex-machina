#include <gtest/gtest.h>
#include "tools/globallog.h"

LOG_INIT("out.txt")


int main(int argc, char** argv)
{
	::testing::InitGoogleTest(&argc, argv);
	int rv = RUN_ALL_TESTS();
	return rv;
}
