/*
 * stringtools.h
 *
 *  Created on: 28 Mar 2015
 *      Author: Ben Cardoen
 */
#ifndef STRINGTOOLS_H_
#define STRINGTOOLS_H_
#include <string>
#include <sstream>

namespace n_tools {

/**
 * Std::string has race errors (COW implementation is not thread safe)
 * Avoid any race by explicitly forcing a string data copy (which is threadsafe).
 * Can be deprecated if using libstdc++ >= 5 or with llvm's libc++.
 */
inline std::string copyString(const std::string& input)
{
	//return std::string(input.data(), input.size());
        return input;
}

/**
 * @brief Test whether the first string ends with the second string.
 * @param full The full string.
 * @param part The possible suffix of full
 */
inline
bool endswith(const std::string& full, const std::string& part)
{
	if (full.length() >= part.length()) {
		return (0
				== full.compare(full.length() - part.length(), part.length(),
						part));
	} else {
		return false;
	}
}

/**
 * @brief Convert unsigned integer to string (Windows friendly)
 * @param i Integer to be converted
 * @return string representation of the integer
 */
inline std::string toString(std::size_t i)
{
#ifndef __CYGWIN__
	return std::to_string(i);
#else
	if (i == 0)
		return "0";
	std::string number = "";
	while (i != 0) {
		char c = (i % 10) + 48; // Black magic with Ascii
		i /= 10;
		number.insert(0, 1, c);
	}
	return number;
#endif
}

/**
 * @brief Convert integer to string (Windows friendly)
 * @param i Integer to be converted
 * @return string representation of the integer
 */
inline std::string toString(int i)
{
#ifndef __CYGWIN__
	return std::to_string(i);
#else
	if (i == 0)
		return "0";
	std::string number = i < 0? "-":"";
	while (i != 0) {
		char c = (i % 10) + 48; // Black magic with Ascii
		i /= 10;
		number.insert(0, 1, c);
	}
	return number;
#endif
}

inline std::string toString(double d){
        std::stringstream ss;
        ss << d;
        return ss.str();
}


/**
 * @brief Convert string to integer (Windows friendly)
 * @param str String to be converted
 * @return integer representation of the string
 */
inline int toInt(std::string str)
{
	int num;
	std::istringstream ss(str);
	ss >> num;
	return num;
}
}

#endif // STRINGTOOLS_H_
