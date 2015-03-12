#include <gtest/gtest.h>
#include<g2log.hpp>
#include<g2logworker.hpp>
#include <std2_make_unique.hpp>

int main(int argc, char** argv)
{
	using namespace g2;
	auto defaultHandler = LogWorker::createWithDefaultLogger(argv[0],".");
	g2::initializeLogging(defaultHandler.worker.get());

	::testing::InitGoogleTest(&argc, argv);
	int rv = RUN_ALL_TESTS();
	//g2::internal::shutDownLogging();
	return rv;

}


