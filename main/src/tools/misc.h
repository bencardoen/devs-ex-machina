/*
 * misc.h
 *
 *  Created on: 27 May 2015
 *      Author: Devs Ex Machina
 */

#ifndef SRC_TOOLS_MISC_H_
#define SRC_TOOLS_MISC_H_

#include <math.h>
#include <type_traits>

namespace n_tools{

/**
 * @brief Signum function
 * Returns 1 if value is positive, -1 if negative, 0 if zero
 */
template <typename T>
constexpr int sgn(T val) {
    return (T(0) < val) - (val < T(0));
}

/**
 * @brief Computes the 2 log of an integral number.
 * @tparam T An integral type.
 * @param val The 2 log will be calculated from this value.
 * @note. The default value may not be too accurate.
 */
template<typename T>
constexpr int intlog2(T val) {
	static_assert(std::is_integral<T>::value, "The type must be an integral type.");
#ifndef __CYGWIN__
	return int(std::log2(double(val)));
#else
	return int(log2(double(val)));
#endif
}

#ifdef __GNUC__
//compiling with g++, use the builtins for better performance.
template<>
constexpr int intlog2<unsigned int>(unsigned int val){
	return sizeof(unsigned int)*8-__builtin_clz(val)-1;
}
template<>
constexpr int intlog2<unsigned long>(unsigned long val){
	return sizeof(unsigned long)*8-__builtin_clzl(val)-1;
}
template<>
constexpr int intlog2<unsigned long long>(unsigned long long val){
	return sizeof(unsigned long long)*8-__builtin_clzll(val)-1;
}

#endif /* __GNUC__ */

} /* namespace n_tools */

#endif /* SRC_TOOLS_MISC_H_ */
