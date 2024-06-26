/*
 * This file is part of the DEVS Ex Machina project.
 * Copyright 2014 - 2015 University of Antwerp
 * https://www.uantwerpen.be/en/
 * Licensed under the EUPL V.1.1
 * A full copy of the license is in COPYING.txt, or can be found at
 * https://joinup.ec.europa.eu/community/eupl/og_page/eupl
 *      Author: Ben Cardoen, Stijn Manhaeve, Tim Tuijn
 */

#ifndef SRC_TOOLS_GLOBALLOG_H_
#define SRC_TOOLS_GLOBALLOG_H_


/// @cond
#define LOG_USEGLOBAL

#ifdef LOG_LEVEL
#if LOG_LEVEL == 0
#define LOGGING false
#else
#define LOGGING true
#endif
#else
#define LOGGING true
#endif
/// @endcond

#include "tools/macros.h"
#include <cstring>
#if LOGGING != false
#include "logger.h"
#include <csignal>
#endif

#ifndef LOG_LEVEL
#define LOG_LEVEL 15	//default logging level
#endif

/// @cond
#if LOG_LEVEL
#define LOGGING true
#endif

#define LOG_ERROR_I	1u
#define LOG_WARNING_I	2u
#define LOG_DEBUG_I	4u
#define LOG_INFO_I	8u

/// @endcond
#define LOG_GLOBAL n_tools::n_globalLog::globalLog

/// @cond
//macros for calling the logging functions
#define LOG_BLOCK(logCommand) do{\
	logCommand;\
}while(0)
#define LOG_NOOP
#define LOG_ARGS(start, ...) start " \t[ ", FILE_SHORT, " L: " STRINGIFY(__LINE__) " F: ", __FUNCTION__, "] \t", __VA_ARGS__, '\n'
#define LOG_CALL(funcname, start, ...) LOG_BLOCK(LOG_GLOBAL.funcname(LOG_ARGS(start, __VA_ARGS__)))

/// @endcond
#if LOG_ERROR_I&LOG_LEVEL
#define LOG_ERROR(...) LOG_CALL(logError, "ERROR", __VA_ARGS__)
#else
#define LOG_ERROR(...) LOG_NOOP
#endif
#if LOG_WARNING_I&LOG_LEVEL
#define LOG_WARNING(...) LOG_CALL(logWarning, "WARNING", __VA_ARGS__)
#else
#define LOG_WARNING(...) LOG_NOOP
#endif
#if LOG_DEBUG_I&LOG_LEVEL
#define LOG_DEBUG(...) LOG_CALL(logDebug, "DEBUG", __VA_ARGS__)
#else
#define LOG_DEBUG(...) LOG_NOOP
#endif
#if LOG_INFO_I&LOG_LEVEL
#define LOG_INFO(...) LOG_CALL(logInfo, "INFO", __VA_ARGS__)
#else
#define LOG_INFO(...) LOG_NOOP
#endif
#if LOG_LEVEL
#define LOG_FLUSH LOG_BLOCK(LOG_GLOBAL.flush())
#else
#define LOG_FLUSH LOG_NOOP
#endif
#if LOG_LEVEL
#define LOG_MOVE(file, append) LOG_BLOCK(LOG_INFO("continuing log in ", file, ". This file will", (append? " NOT ": " "), "be overwritten.");LOG_GLOBAL.logMove(file, append))
#else
#define LOG_MOVE(file, append) LOG_NOOP
#endif
#if LOG_LEVEL
#define LOG_ARGV(argc, argv) \
	LOG_BLOCK(LOG_GLOBAL.logDebug("DEBUG" " \t[ ", FILE_SHORT, " L: " STRINGIFY(__LINE__) " F: ", __FUNCTION__, "] \t");\
	for(int i = 0; i < argc; ++i) { LOG_GLOBAL.logDebug(argv[i], ' ');} \
	LOG_GLOBAL.logDebug('\n');)
#else
#define LOG_ARGV(argc, argv) LOG_NOOP
#endif

//macro for intitializing the global logger
#if LOGGING==true

/// @cond
namespace n_tools {
namespace n_globalLog {

extern Logger<LOG_LEVEL> globalLog;

} /*namespace n_globalLog*/
} /*namespace n_tools*/

/// @endcond

#define LOG_INIT(filename) n_tools::Logger<LOG_LEVEL> LOG_GLOBAL(filename);
#else
#define LOG_INIT(filename)
#endif

/**
 * @def LOG_ERROR(...)
 * @brief Logs a message with the Error log level.
 * @param ... a comma-seperated list of objects/values that will be logged.
 * @see Logger
 */
/**
 * @def LOG_WARNING(...)
 * @brief Logs a message with the Warning log level.
 * @param ... a comma-seperated list of objects/values that will be logged.
 * @see Logger
 */
/**
 * @def LOG_DEBUG(...)
 * @brief Logs a message with the Debug log level.
 * @param ... a comma-seperated list of objects/values that will be logged.
 * @see Logger
 */
/**
 * @def LOG_INFO(...)
 * @brief Logs a message with the Info log level.
 * @param ... a comma-seperated list of objects/values that will be logged.
 * @see Logger
 */
/**
 * @def LOG_INIT(filename)
 * @brief Initializes the global logger.
 * @param filename The path to the file where the log is written to.
 * @see Logger
 */
/**
 * @def LOG_LEVEL
 * @brief The logging level filter used by the global logger.
 * @see Logger
 */
/**
 * @def LOG_FLUSH
 * @brief Flushes the logger, forcing it to empty its buffers and write the data to the opened file.
 * @see Logger::flush
 */
/**
 * @def LOG_GLOBAL
 * @brief Gives access to the global Logger object, if it exists.
 * @see Logger
 */
 /**
  * @def LOG_MOVE
  * @brief Starts outputting log data to a new file.
  * @see Logger::logMove
  * @note Calling this function in a parallel program when other threads are accessing the logs will be safe, but might put unwanted data in either file.
  */

#endif /* SRC_TOOLS_GLOBALLOG_H_ */
