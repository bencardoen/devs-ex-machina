/*
 * stringtools.h
 *
 *  Created on: 28 Mar 2015
 *      Author: Ben Cardoen
 */
#ifndef STRINGTOOLS_H_
#define STRINGTOOLS_H_
#include <string>

namespace n_tools{


/**
 * Std::string has race errors (COW implementation is not thread safe)
 * Avoid any race by explicitly forcing a string data copy (which is threadsafe).
 */
inline
std::string copyString(const std::string& input){
	return std::string(input.data(), input.size());
}

/**
 * @brief Test whether the first string ends with the second string.
 * @param full The full string.
 * @param part The possible suffix of full
 */
inline
bool endswith(const std::string& full, const std::string& part){
	if (full.length() >= part.length()) {
	return (0 == full.compare (full.length() - part.length(), part.length(), part));
    } else {
	return false;
    }
}

}

#endif // STRINGTOOLS_H_
