#include <gtest/gtest.h>
#include<g2log.hpp>
#include<g2logworker.hpp>
#include <std2_make_unique.hpp>

int main(int argc, char** argv)
{
	::testing::InitGoogleTest(&argc, argv);
	int rv = RUN_ALL_TESTS();
	return rv;

}


