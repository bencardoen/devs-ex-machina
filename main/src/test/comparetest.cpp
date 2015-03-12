/*
 * comparetest.cpp
 *
 *  Created on: Mar 12, 2015
 *      Author: Stijn Manhaeve - Devs Ex Machina
 */

#include <gtest/gtest.h>
#include <sstream>
#include "compare.h"

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

void basicStreamcmpTest(const char* str1, const char* str2){
	std::istringstream* stream1 = new std::istringstream(str1);
	std::istringstream* stream2 = new std::istringstream(str2);
	//std::cout << ">str1:" << str1 << "\n str2:" << str2 << "\n";
	EXPECT_EQ(streamcmp(*stream1, *stream2) , conststrcmp(str1, str2));
	delete stream1;
	delete stream2;
}

TEST(Compare, streamcmp){
	basicStreamcmpTest("a", "a");
	basicStreamcmpTest("", "");
	basicStreamcmpTest("abcdef", "abcdef");
	basicStreamcmpTest("cdef", "abcdef");
	basicStreamcmpTest("abcdef", "cdef");
	basicStreamcmpTest("abcdef", "");
	basicStreamcmpTest("", "cdef");
}

//small shortcut to the folder where the test files are located.
//Using a macro allows me to concatenate string literals easily without having to mess with std::string
#define TESTFOLDER "testfiles/compare/"

TEST(Compare, filecmp){
	EXPECT_EQ(filecmp(TESTFOLDER"file0_a.txt", TESTFOLDER"file0_a.txt"), 0);
	EXPECT_EQ(filecmp(TESTFOLDER"file0_a.txt", TESTFOLDER"file0_b.txt"), 0);
	EXPECT_EQ(filecmp(TESTFOLDER"file0_a.txt", TESTFOLDER"file1.txt"), 't' - 'L');
	EXPECT_EQ(filecmp(TESTFOLDER"file1.txt", TESTFOLDER"file0_a.txt"), 'L' - 't');
	EXPECT_EQ(filecmp(TESTFOLDER"file1_empty.txt", TESTFOLDER"file0_a.txt"), -'t');
	EXPECT_EQ(filecmp(TESTFOLDER"file0_a.txt", TESTFOLDER"file1_empty.txt"), 't');
	EXPECT_THROW(filecmp(TESTFOLDER"file0_a.txt", "IDontExist.txt"), std::ios_base::failure);
	EXPECT_THROW(filecmp("IDontExist.txt", TESTFOLDER"file0_a.txt"), std::ios_base::failure);
	EXPECT_THROW(filecmp("IDontExist.txt", "meNeither.txt"), std::ios_base::failure);
	EXPECT_THROW(filecmp(nullptr, TESTFOLDER"file0_a.txt"), std::ios_base::failure);
	EXPECT_THROW(filecmp("", TESTFOLDER"file0_a.txt"), std::ios_base::failure);
}

//undefine the macro for good measure, even though it shouldn't be necessary as this is not a header file.
#undef TESTFOLDER
