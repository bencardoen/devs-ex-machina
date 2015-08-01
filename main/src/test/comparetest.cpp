/*
 * comparetest.cpp
 *
 *  Created on: Mar 12, 2015
 *      Author: Stijn Manhaeve - Devs Ex Machina
 */

#include <gtest/gtest.h>
#include <sstream>
#include "test/compare.h"

using namespace n_misc;

TEST(Compare, charcmp){
	EXPECT_EQ(charcmp('a', 'a'), 0);
	EXPECT_EQ(charcmp('a', 'b'), -1);
	EXPECT_EQ(charcmp('b', 'a'), 1);
	EXPECT_EQ(charcmp(0, 'b'), -int('b'));
	EXPECT_EQ(charcmp('a', 0), int('a'));
	EXPECT_EQ(charcmp('a', 'z') < 0, true);
	EXPECT_EQ(charcmp((char)0, (char)127), -127);
	EXPECT_EQ(charcmp((char)0, (char)255), -255);
}

TEST(Compare, strcmp){
	EXPECT_EQ(conststrcmp("", ""), 0);
	EXPECT_EQ(conststrcmp("a", "a"), 0);
	EXPECT_EQ(conststrcmp("ab", "ab"), 0);
	EXPECT_EQ(conststrcmp("abc", "abc"), 0);
	EXPECT_EQ(conststrcmp("a", "b"), -1);
	EXPECT_EQ(conststrcmp("a", "ab") < 0, true);
	EXPECT_EQ(conststrcmp("", "b"), -int('b'));
	EXPECT_EQ(conststrcmp("a", ""), int('a'));
	//the following lines compile but should throw an error to prevent segmentation faults
	EXPECT_THROW(conststrcmp("a", nullptr), std::logic_error);
	EXPECT_THROW(conststrcmp(nullptr, "b"), std::logic_error);
	EXPECT_THROW(conststrcmp(nullptr, nullptr), std::logic_error);
}

int basicStreamcmp(const char* str1, const char* str2, bool skipWhitespace = false){
	std::istringstream* stream1 = new std::istringstream(str1);
	std::istringstream* stream2 = new std::istringstream(str2);

	int value = streamcmp(*stream1, *stream2, skipWhitespace);

	delete stream1;
	delete stream2;
	return value;
}

void basicStreamcmpTest(const char* str1, const char* str2){
	EXPECT_EQ(basicStreamcmp(str1, str2) , conststrcmp(str1, str2));
}

TEST(Compare, streamcmp){
	basicStreamcmpTest("a", "a");
	basicStreamcmpTest("", "");
	basicStreamcmpTest("abcdef", "abcdef");
	basicStreamcmpTest("cdef", "abcdef");
	basicStreamcmpTest("abcdef", "cdef");
	basicStreamcmpTest("abcdef", "");
	basicStreamcmpTest("", "cdef");
	EXPECT_EQ(basicStreamcmp("abcdef", "ab cdef", true), 0);
	EXPECT_NE(basicStreamcmp("abcdef", "abc def", false), 0);
}

#define TESTFOLDERCOMP TESTFOLDER"compare/"

TEST(Compare, filecmp){
	EXPECT_EQ(filecmp(TESTFOLDERCOMP"file0_a.txt", TESTFOLDERCOMP"file0_a.txt", false), 0);
	EXPECT_EQ(filecmp(TESTFOLDERCOMP"file0_a.txt", TESTFOLDERCOMP"file0_b.txt", false), 0);
	EXPECT_NE(filecmp(TESTFOLDERCOMP"file0_a.txt", TESTFOLDERCOMP"file0_c.txt", false), 0);
	EXPECT_EQ(filecmp(TESTFOLDERCOMP"file0_a.txt", TESTFOLDERCOMP"file0_c.txt", true), 0);
	EXPECT_EQ(filecmp(TESTFOLDERCOMP"file0_a.txt", TESTFOLDERCOMP"file1.txt", false), 't' - 'L');
	EXPECT_EQ(filecmp(TESTFOLDERCOMP"file1.txt", TESTFOLDERCOMP"file0_a.txt", false), 'L' - 't');
	EXPECT_EQ(filecmp(TESTFOLDERCOMP"file1_empty.txt", TESTFOLDERCOMP"file0_a.txt", false), -'t');
	EXPECT_EQ(filecmp(TESTFOLDERCOMP"file0_a.txt", TESTFOLDERCOMP"file1_empty.txt", false), 't');
	EXPECT_THROW(filecmp(TESTFOLDERCOMP"file0_a.txt", "IDontExist.txt", false), std::ios_base::failure);
	EXPECT_THROW(filecmp("IDontExist.txt", TESTFOLDERCOMP"file0_a.txt", false), std::ios_base::failure);
	EXPECT_THROW(filecmp("IDontExist.txt", "meNeither.txt", false), std::ios_base::failure);
	EXPECT_THROW(filecmp(nullptr, TESTFOLDERCOMP"file0_a.txt", false), std::ios_base::failure);
	EXPECT_THROW(filecmp("", TESTFOLDERCOMP"file0_a.txt", false), std::ios_base::failure);
}

//undefine the macro for good measure, even though it shouldn't be necessary as this is not a header file.
#undef TESTFOLDER
