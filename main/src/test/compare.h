/*
 * compare.h
 *
 *  Created on: Mar 12, 2015
 *      Author: Stijn Manhaeve - Devs Ex Machina
 */

#ifndef SRC_TEST_COMPARE_H_
#define SRC_TEST_COMPARE_H_

#include <fstream>
#include <stdexcept>

namespace n_misc{
/**
 * @brief compile time char comparison
 * @param c1 the first character
 * @param c2 the second character
 * @retval <0 if c1 < c2
 * @retval 0 if c1 == c2
 * @retval >0 if c1 > c2
 * @note This implementation forces the char type to be unsigned. If it isn't, the values are first converted.
 * 			The reason is that some implementations of char are signed and others are unsigned. The same method might therefore give different results across those platforms.
 */
constexpr int charcmp(char c1, char c2){
	return (int((unsigned char)c1) - int((unsigned char)c2));
}
/**
 * @brief compile time c-string comparison. (@see std::strcmp)
 * @param str1 first c-style string
 * @param str2 second c-style string
 * @retval <0 if a < b With a, b = the first different character in str1, resp. str2
 * @retval 0 if both strings are the same.
 * @retval >0 if a > b With a, b = the first different character in str1, resp. str2
 * @throws std::logic_error If one or both of the arguments are nullptr. This is to make debugging a lot easier
 * 			    and to prevent blunt segmentation faults
 */
constexpr int conststrcmp(const char* str1, const char* str2){
	return (str1 == nullptr || str2 == nullptr)?(throw std::logic_error("conststrcmp: at least on of the arguments is a nullptr")):
		((*str1 != *str2)?
			charcmp(*str1, *str2): ((*str1 == 0)?
				0: conststrcmp(str1+1, str2+1)));
}

/**
 * @brief compares the contents of two input streams.
 * @param str1 The first input stream
 * @param str2 The second input stream
 * @retval <0 if a < b With a, b = the first different character in str1, resp. str2
 * @retval 0 if both strings are the same.
 * @retval >0 if a > b With a, b = the first different character in str1, resp. str2
 * @throws std::ios_base::failure if there is a problem with the streams.
 * @note All input is consumed until either of the streams has ended (eof)
 */
int streamcmp(std::istream& str1, std::istream& str2, bool skipWhitespace = true);

/**
 * @brief compares the contents of two files.
 * @param file1 A c-style string containing the filename of the first file.
 * @param file2 A c-style string containing the filename of the second file.
 * @throws std::ios_base::failure if an error arises, e.g. if a file is not readable.
 */
int filecmp(const char* file1, const char* file2, bool skipWhitespace = true);

}/*namespace n_misc*/


//small shortcut to the folder where the test files are located.
//Using a macro allows us to concatenate string literals easily without having to mess with std::string
#define TESTFOLDER "testfiles/"

#endif /* SRC_TEST_COMPARE_H_ */
