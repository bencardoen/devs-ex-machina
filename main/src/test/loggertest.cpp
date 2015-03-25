/*
 * loggertest.cpp
 *
 *  Created on: Mar 22, 2015
 *      Author: Stijn Manhaeve - Devs Ex Machina
 */

#include <gtest/gtest.h>
#include "logger.h"
#include "compare.h"
#include "macros.h"

using namespace n_tools;

const char* c = TESTFOLDER;
#define TESTFOLDERLOG TESTFOLDER"logging/"

#define DOTEST(I) TEST(Logger, Logging##I)\
{\
	{\
		Logger<I> logger(TESTFOLDERLOG "logging" STRINGIFY(I) ".out");\
		logger.logDebug("DEBUG log 1\n");\
		logger.logError("ERROR log 1\n");\
		logger.logInfo("INFO log 1\n");\
		logger.logWarning("WARNING log 2\n");\
		logger.logDebug("DEBUG log 2\n");\
		logger.logError("ERROR log 2\n");\
		logger.logInfo("INFO log 2\n");\
		logger.logWarning("WARNING log 2\n");\
	}\
	EXPECT_EQ(n_misc::filecmp(TESTFOLDERLOG "logging" STRINGIFY(I) ".out", TESTFOLDERLOG "logging" STRINGIFY(I) ".corr"), 0);\
}

DOTEST(0)
DOTEST(1)
DOTEST(2)
DOTEST(3)
DOTEST(4)
DOTEST(5)
DOTEST(6)
DOTEST(7)
DOTEST(8)
DOTEST(9)
DOTEST(10)
DOTEST(11)
DOTEST(12)
DOTEST(13)
DOTEST(14)
DOTEST(15)
