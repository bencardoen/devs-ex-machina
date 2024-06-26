/*
 * This file is part of the DEVS Ex Machina project.
 * Copyright 2014 - 2015 University of Antwerp
 * https://www.uantwerpen.be/en/
 * Licensed under the EUPL V.1.1
 * A full copy of the license is in COPYING.txt, or can be found at
 * https://joinup.ec.europa.eu/community/eupl/og_page/eupl
 *      Author: Stijn Manhaeve
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

//If NO_TRACER is defined, don't use the tracer module
#ifndef NO_TRACER
#define NO_TRACER
#endif /* NO_TRACER */

//If USE_SAFE_CAST is true, use safe casts (dynamic casts)
//otherwise, use unsafe, fast casts (reinterpret cast)
#ifndef USE_SAFE_CAST
#define USE_SAFE_CAST false
#endif /* USE_SAFE_CAST */

#else /* BENCHMARK */
#ifndef USE_STAT
#define USE_STAT
#endif /* USE_STAT */

#ifndef USE_SAFE_CAST
#define USE_SAFE_CAST true
#endif /* USE_SAFE_CAST */

#endif /* BENCHMARK */

#endif /* SRC_TOOLS_MACROS_H_ */
