/*
 * globallog.h
 *
 *  Created on: Mar 14, 2015
 *      Author: Stijn Manhaeve - Devs Ex Machina
 */
#ifndef SRC_TOOLS_GLOBALLOG_H_
#define SRC_TOOLS_GLOBALLOG_H_
#include "logger.h"

//MACRO's
//define macros for the message level
#define LOG_NONE		0
#define LOG_ERROR_I		1
#define LOG_WARNING_I	2
#define LOG_DEBUG_I		4
#define LOG_INFO_I		8
#define LOG_ALL_I		LOG_INFO_I

#define LOG_ERROR_STR	"ERROR"
#define LOG_WARNING_STR	"WARNING"
#define LOG_DEBUG_STR	"DEBUG"
#define LOG_INFO_STR	"INFO"

//define macros for the current log level filter
#ifndef LOG_LEVEL
#define LOG_LEVEL LOG_INFO_I
#endif

#ifdef LOG_LEVEL
#if LOG_LEVEL==LOG_INFO_I
#define LOG_FILTER 15
#define LOGGING true
#elif LOG_LEVEL==LOG_DEBUG_I
#define LOG_FILTER 7
#define LOGGING true
#elif LOG_LEVEL==LOG_WARNING_I
#define LOG_FILTER 3
#define LOGGING true
#elif LOG_LEVEL==LOG_ERROR_I
#define LOG_FILTER 1
#define LOGGING true
#elif LOG_LEVEL!=0
#define LOG_FILTER LOG_LEVEL
#define LOGGING true
#else
#define LOG_FILTER 0
#define LOGGING false
#endif
#else
#error Cannot use globallog if LogLevel is not defined
#endif

namespace n_tools{
namespace n_globalLog{

extern Logger globalLog;
extern std::mutex globalLogMutex;

}	/*namespace n_globalLog*/
}	/*namespace n_tools*/


#define LOG_GLOBAL n_tools::n_globalLog::globalLog
#define LOG_MUTEX n_tools::n_globalLog::globalLogMutex

//macro for intitializing the global logger
#if LOGGING==true
#define LOG_INIT(filename) n_tools::Logger LOG_GLOBAL(filename, LOG_FILTER); std::mutex LOG_MUTEX;
#else
#define LOG_INIT(filename)
#endif
//macros for calling the logging functions
#define LOG_BLOCK(logCommand) do{std::lock_guard<std::mutex> lock(LOG_MUTEX); logCommand}while(0)
#define LOG_NOOP do{}while(0)
#define LOG_ARGS(start, data) LOG_GLOBAL << start << " \t[ "<< __FILE__ << " L: " << __LINE__ << "] \t" << data << '\n';

#if LOG_ERROR_I&LOG_FILTER
#define LOG_ERROR(data) LOG_BLOCK(LOG_ARGS(LOG_ERROR_STR, data))
#else
#define LOG_ERROR(data) LOG_NOOP
#endif
#if LOG_WARNING_I&LOG_FILTER
#define LOG_WARNING(data) LOG_BLOCK(LOG_ARGS(LOG_WARNING_STR, data))
#else
#define LOG_WARNING(data) LOG_NOOP
#endif
#if LOG_DEBUG_I&LOG_FILTER
#define LOG_DEBUG(data) LOG_BLOCK(LOG_ARGS(LOG_DEBUG_STR, data))
#else
#define LOG_DEBUG(data) LOG_NOOP
#endif
#if LOG_INFO_I&LOG_FILTER
#define LOG_INFO(data) LOG_BLOCK(LOG_ARGS(LOG_INFO_STR, data))
#else
#define LOG_INFO(data) LOG_NOOP
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
