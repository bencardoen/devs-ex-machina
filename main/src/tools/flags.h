/*

 * flags.h
 *	Basic functions for working with bit flags, used in threadsignalling.
 * 	Created on: 30 Apr 2015
 *	Author: Ben Cardoen
 */

#ifndef SRC_TOOLS_FLAGS_H_
#define SRC_TOOLS_FLAGS_H_

namespace n_tools{

template<typename T=std::size_t>
bool flag_is_set(const T& testvalue, const T& flag){
	return ((testvalue & flag)==flag);
}

template<typename T=std::size_t>
void unset_flag(T& testvalue, const T& flag){
	testvalue &= ~flag;
}

template<typename T=std::size_t>
void set_flag(T& testvalue, const T& flag){
	testvalue |= flag;
}

}
#endif /* SRC_TOOLS_FLAGS_H_ */
