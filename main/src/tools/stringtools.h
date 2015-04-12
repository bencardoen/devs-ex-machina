/*
 * stringtools.h
 *
 *  Created on: 28 Mar 2015
 *      Author: Ben Cardoen
 */

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

}

