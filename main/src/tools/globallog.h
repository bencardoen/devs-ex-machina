/*
 * globallog.h
 *
 *  Created on: Mar 14, 2015
 *      Author: Stijn Manhaeve - Devs Ex Machina
 */
#ifndef SRC_TOOLS_GLOBALLOG_H_
#define SRC_TOOLS_GLOBALLOG_H_
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



#include "macros.h"
#include <cstring>
#if LOGGING != false
#include "logger.h"
#include <csignal>
#endif

#ifndef LOG_LEVEL
#define LOG_LEVEL 15	//default logging level
#endif

#if LOG_LEVEL
#define LOGGING true
#endif

#define LOG_ERROR_I	1u
#define LOG_WARNING_I	2u
#define LOG_DEBUG_I	4u
#define LOG_INFO_I	8u

#define LOG_GLOBAL n_tools::n_globalLog::globalLog

//macros for calling the logging functions
#define LOG_BLOCK(logCommand) do{\
	logCommand;\
}while(0)
#define LOG_NOOP
#define LOG_ARGS(start, ...) start " \t[ ", FILE_SHORT, " L: " STRINGIFY(__LINE__) "] \t", __VA_ARGS__, '\n'
#define LOG_CALL(funcname, start, ...) LOG_BLOCK(LOG_GLOBAL.funcname(LOG_ARGS(start, __VA_ARGS__)))

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

//macro for intitializing the global logger
#if LOGGING==true

namespace n_tools {
namespace n_globalLog {

extern Logger<LOG_LEVEL> globalLog;

} /*namespace n_globalLog*/
} /*namespace n_tools*/
#define LOG_INIT(filename) n_tools::Logger<LOG_LEVEL> LOG_GLOBAL(filename);
#else
#define LOG_INIT(filename)
#endif

//TODO clean up macro definitions
//#undef LOG_NONE
//#undef LOG_ERROR_I
//#undef LOG_WARNING_I
//#undef LOG_DEBUG_I
//#undef LOG_INFO_I
//
//#undef LOG_ERROR_STR
//#undef LOG_WARNING_STR
//#undef LOG_DEBUG_STR
//#undef LOG_INFO_STR
//#undef LOG_LEVEL
//#undef LOG_FILTER
//#undef LOG_BLOCK
//#undef LOG_ARGS
//#undef LOG_NOOP
//
//#undef LOG_GLOBAL
//#undef LOG_MUTEX
#endif /* SRC_TOOLS_GLOBALLOG_H_ */
