/*
 * macros.h
 *
 *  Created on: Mar 19, 2015
 *      Author: Stijn Manhaeve - Devs Ex Machina
 */

#ifndef SRC_TOOLS_MACROS_H_
#define SRC_TOOLS_MACROS_H_


/**
 * @brief The macro value is a computation for the short filename (without the filepath)
 * @see http://stackoverflow.com/a/8488201
 */
#define FILE_SHORT (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

#define STRINGIFY_IMPL(arg) #arg
/**
 * @brief Converts any macro value to a string literal.
 */
#define STRINGIFY(arg) STRINGIFY_IMPL(arg)

#endif /* SRC_TOOLS_MACROS_H_ */
