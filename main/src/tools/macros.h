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

#define GCC_VERSION (__GNUC__ * 10000 \
                               + __GNUC_MINOR__ * 100 \
                               + __GNUC_PATCHLEVEL__)
#if GCC_VERSION<50000
#ifndef USE_COPY_STRING
#define USE_COPY_STRING
#endif /* USE_COPY_STRING */
#endif /* GCC_VERSION <= 50000 */

#ifdef BENCHMARK

#ifndef NO_TRACER
#define NO_TRACER
#endif /* NO_TRACER */

#else /* BENCHMARK */
#ifndef USE_STAT
#define USE_STAT
#endif /* USE_STAT */

#endif /* BENCHMARK */

#endif /* SRC_TOOLS_MACROS_H_ */
