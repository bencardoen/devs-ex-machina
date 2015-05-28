/*
 * misc.h
 *
 *  Created on: 27 May 2015
 *      Author: matthijs
 */

#ifndef SRC_TOOLS_MISC_H_
#define SRC_TOOLS_MISC_H_

namespace n_tools{

/**
 * @brief Signum function
 * Returns 1 if value is positive, -1 if negative, 0 if zero
 */
template <typename T> int sgn(T val) {
    return (T(0) < val) - (val < T(0));
}

} /* namespace n_tools */

#endif /* SRC_TOOLS_MISC_H_ */
