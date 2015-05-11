#include <gtest/gtest.h>
#include "globallog.h"

LOG_INIT("out.txt")

int main(int argc, char** argv)
{

	::testing::InitGoogleTest(&argc, argv);
	// gtest intercepts exceptions, else we need try/catch to force stackunwind.
	int rv = RUN_ALL_TESTS();
	return rv;
}
