/*
 * logger.h
 *
 *  Created on: Mar 14, 2015
 *      Author: Stijn Manhaeve - Devs Ex Machina
 */

#ifndef SRC_TOOLS_LOGGER_H_
#define SRC_TOOLS_LOGGER_H_

#include <condition_variable>
#include <fstream>
#include <mutex>
#include <queue>
#include <streambuf>
#include <string>
#include <thread>
#include <vector>
#include "tools/asynchwriter.h"

namespace n_tools {

enum LoggingLevel
{
	E_ERROR = 1, E_WARNING = 2, E_DEBUG = 4, E_INFO = 8
};

template<unsigned int logFilter = 15>
class Logger
{
public:
	Logger(const std::string& filename)
		: m_filename(filename)
		, m_buf(new ASynchWriter(filename))
		, m_out(m_buf)
	{
		m_out.rdbuf(m_buf);
	}
	~Logger() {
		//Probably don't need to lock here.
		//If you try to print stuff while destructing the logger, it's your own fault that stuff will fail
		m_out.flush();
		delete m_buf;
	}

	template<typename... Args>
	typename std::enable_if<(logFilter & E_ERROR) && sizeof...(Args), void>::type
	logError(const Args&... args){
		std::lock_guard<std::mutex> m(this->m_mutex);	//unsure whether this is necessary
		logImpl(args...);
	}
	template<typename... Args>
	typename std::enable_if<!(logFilter & E_ERROR) && sizeof...(Args), void>::type
	logError(const Args&...){
		//don't log this level
	}

	template<typename... Args>
	typename std::enable_if<(logFilter & E_WARNING) && sizeof...(Args), void>::type
	logWarning(const Args&... args){
		std::lock_guard<std::mutex> m(this->m_mutex);	//unsure whether this is necessary
		logImpl(args...);
	}
	template<typename... Args>
	typename std::enable_if<!(logFilter & E_WARNING) && sizeof...(Args), void>::type
	logWarning(const Args&...){
		//don't log this level
	}

	template<typename... Args>
	typename std::enable_if<(logFilter & E_DEBUG) && sizeof...(Args), void>::type
	logDebug(const Args&... args){
		std::lock_guard<std::mutex> m(this->m_mutex);	//unsure whether this is necessary
		logImpl(args...);
	}
	template<typename... Args>
	typename std::enable_if<!(logFilter & E_DEBUG) && sizeof...(Args), void>::type
	logDebug(const Args&...){
		//don't log this level
	}

	template<typename... Args>
	typename std::enable_if<(logFilter & E_INFO) && sizeof...(Args), void>::type
	logInfo(const Args&... args){
		std::lock_guard<std::mutex> m(this->m_mutex);	//unsure whether this is necessary
		logImpl(args...);
	}
	template<typename... Args>
	typename std::enable_if<!(logFilter & E_INFO) && sizeof...(Args), void>::type
	logInfo(const Args&...){
		//don't log this level
	}

	/**
	 * @brief Flushes all remaining output and forces a write to file
	 */
	void flush(){
		std::lock_guard<std::mutex> m(this->m_mutex);	//DEFINITELY lock here as we're deleting the buffer!
		m_out.flush();
		delete m_buf;
		m_buf = new ASynchWriter(m_filename, std::ios_base::app | std::ios_base::out);	//will append
		m_out.rdbuf(m_buf);
	}

private:
	std::string m_filename;
	ASynchWriter* m_buf;
	std::ostream m_out;
	std::mutex m_mutex;

	template<typename T, typename... Args>
	void logImpl(const T& t, const Args&... args){
		m_out << t;
		logImpl(args...);
	}

	template<typename... Args>
	void logImpl(){
		//nothing to do here
	}

};

std::ostream& operator<<(std::ostream& out, LoggingLevel level);

} /* namespace n_tools */

#include "forwarddeclare/logger.h"

#endif /* SRC_TOOLS_LOGGER_H_ */
