/*
 * This file is part of the DEVS Ex Machina project.
 * Copyright 2014 - 2015 University of Antwerp
 * https://www.uantwerpen.be/en/
 * Licensed under the EUPL V.1.1
 * A full copy of the license is in COPYING.txt, or can be found at
 * https://joinup.ec.europa.eu/community/eupl/og_page/eupl
 *      Author: Ben Cardoen, Stijn Manhaeve, Tim Tuijn, Matthijs Van Os
 */

#ifndef STRINGTOOLS_H_
#define STRINGTOOLS_H_
#include <string>
#include <cstring>
#include <sstream>
#include "tools/macros.h"

namespace n_tools {

/**
 * std::string has race errors (COW implementation is not thread safe)
 * Avoid any race by explicitly forcing a string data copy (which is threadsafe).
 * Can be deprecated if using libstdc++ >= 5 or with llvm's libc++.
 */
#ifdef USE_COPY_STRING
inline std::string copyString(const std::string& input)
{
                return std::string(input.data(),input.size());
}
#else /* USE_COPY_STRING */
constexpr const std::string& copyString(const std::string& input)
{
                return input;
}
#endif /* USE_COPY_STRING */

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


template<typename T>
inline std::string toString(T d){
        std::stringstream ss;
        ss << d;
        return ss.str();
}

/**
 * @brief Convert unsigned integer to string (Windows friendly)
 * @param i Integer to be converted
 * @return string representation of the integer
 */
template<>
inline std::string toString<std::size_t>(std::size_t i)
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
template<>
inline std::string toString<int>(int i)
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

template<typename T>
T toData(std::string str)
{
	T num;
	std::istringstream ss(str);
	ss >> num;
	return num;
}

inline
char getOpt(char* argv){
	if(strlen(argv) == 2 && argv[0] == '-')
		return argv[1];
	return 0;
}
}

#endif // STRINGTOOLS_H_
