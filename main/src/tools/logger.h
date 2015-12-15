/*
 * This file is part of the DEVS Ex Machina project.
 * Copyright 2014 - 2015 University of Antwerp
 * https://www.uantwerpen.be/en/
 * Licensed under the EUPL V.1.1
 * A full copy of the license is in COPYING.txt, or can be found at
 * https://joinup.ec.europa.eu/community/eupl/og_page/eupl
 *      Author: Ben Cardoen, Stijn Manhaeve
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

/**
 * @brief Constants used for identifying the different log levels.
 *
 * These levels can be combined for creating log message filters.
 */
enum LoggingLevel
{
	/** Error level. This log level is reserved for reporting errors.*/
	E_ERROR = 1,
	/** Warning level. This log level is reserved for potential errors and other warnings.*/
	E_WARNING = 2,
	/** Debug level. For creating debug information.*/
	E_DEBUG = 4,
	/** Info level, for uses that not fit in with any of the previous log levels.*/
	E_INFO = 8
};

/**
 * @brief Class responsible for logging messages.
 * @tparam logFilter A filter for log messages.
 * 		If the logging level of a message does not pass the filter,
 * 		the message is not logged.
 * @see LoggingLevel
 */
template<unsigned int logFilter = 15>
class Logger
{
public:
	/**
	 * @brief Creates a new logger that will write the data to a file.
	 * @param filename The path to the file where the log is written to.
	 */
	Logger(const std::string& filename)
		: m_filename(filename)
		, m_buf(new ASynchWriter(filename))
		, m_out(m_buf)
	{
		m_out.rdbuf(m_buf);
	}

	/**
	 * @brief Destructor. Will release all resources associated with the output file.
	 */
	~Logger() {
		//Probably don't need to lock here.
		//If you try to print stuff while destructing the logger, it's your own fault that stuff will fail
		m_out.flush();
		delete m_buf;
	}

	/**
	 * @brief Prints a message with the Error log level.
	 * @param args... All arguments are written to the file one by one.
	 * @precondition At least one argument is given.
	 */
	template<typename... Args>
	typename std::enable_if<(logFilter & E_ERROR) && sizeof...(Args), void>::type
	logError(const Args&... args){
		std::lock_guard<std::mutex> m(this->m_mutex);	//unsure whether this is necessary
		logImpl(args...);
	}

	/**
	 * @brief Prints a message with the Error log level.
	 * @param args... All arguments are written to the file one by one.
	 * @precondition At least one argument is given.
	 */
	template<typename... Args>
	typename std::enable_if<!(logFilter & E_ERROR) && sizeof...(Args), void>::type
	logError(const Args&...){
		//don't log this level
	}

	/**
	 * @brief Prints a message with the Warning log level.
	 * @param args... All arguments are written to the file one by one.
	 * @precondition At least one argument is given.
	 */
	template<typename... Args>
	typename std::enable_if<(logFilter & E_WARNING) && sizeof...(Args), void>::type
	logWarning(const Args&... args){
		std::lock_guard<std::mutex> m(this->m_mutex);	//unsure whether this is necessary
		logImpl(args...);
	}

	/**
	 * @brief Prints a message with the Warning log level.
	 * @param args... All arguments are written to the file one by one.
	 * @precondition At least one argument is given.
	 */
	template<typename... Args>
	typename std::enable_if<!(logFilter & E_WARNING) && sizeof...(Args), void>::type
	logWarning(const Args&...){
		//don't log this level
	}

	/**
	 * @brief Prints a message with the Debug log level.
	 * @param args... All arguments are written to the file one by one.
	 * @precondition At least one argument is given.
	 */
	template<typename... Args>
	typename std::enable_if<(logFilter & E_DEBUG) && sizeof...(Args), void>::type
	logDebug(const Args&... args){
		std::lock_guard<std::mutex> m(this->m_mutex);	//unsure whether this is necessary
		logImpl(args...);
	}

	/**
	 * @brief Prints a message with the Debug log level.
	 * @param args... All arguments are written to the file one by one.
	 * @precondition At least one argument is given.
	 */
	template<typename... Args>
	typename std::enable_if<!(logFilter & E_DEBUG) && sizeof...(Args), void>::type
	logDebug(const Args&...){
		//don't log this level
	}

	/**
	 * @brief Prints a message with the Info log level.
	 * @param args... All arguments are written to the file one by one.
	 * @precondition At least one argument is given.
	 */
	template<typename... Args>
	typename std::enable_if<(logFilter & E_INFO) && sizeof...(Args), void>::type
	logInfo(const Args&... args){
		std::lock_guard<std::mutex> m(this->m_mutex);	//unsure whether this is necessary
		logImpl(args...);
	}

	/**
	 * @brief Prints a message with the Info log level.
	 * @param args... All arguments are written to the file one by one.
	 * @precondition At least one argument is given.
	 */
	template<typename... Args>
	typename std::enable_if<!(logFilter & E_INFO) && sizeof...(Args), void>::type
	logInfo(const Args&...){
		//don't log this level
	}

	/**
	 * @brief Flushes all remaining output and forces a write to the file system.
	 */
	void flush(){
		std::lock_guard<std::mutex> m(this->m_mutex);	//DEFINITELY lock here as we're deleting the buffer!
		m_out.flush();
		delete m_buf;
		m_buf = new ASynchWriter(m_filename, std::ios_base::app | std::ios_base::out);	//will append
		m_out.rdbuf(m_buf);
	}

	/**
	 * @brief Start writing to a different log file.
	 * @param newFile The filename of the new file.
	 * @param append [default = false]  If true, append subsequent log calls to the file.
	 *                                  Otherwise, replace the file if it exists already.
	 * Calling this function in a parallel program when other threads are accessing the logs will be safe, but might put unwanted data in either file.
	 */
	void logMove(const std::string& newFile, bool append=false){
	        std::lock_guard<std::mutex> m(this->m_mutex);   //DEFINITELY lock here as we're deleting the buffer!
	        m_out.flush();
	        delete m_buf;
	        m_filename = newFile;
	        m_buf = new ASynchWriter(m_filename, append? (std::ios_base::app | std::ios_base::out): std::ios_base::out);  //will append
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

/**
 * @brief output operator overload for LoggingLevel.
 * @param out The std::ostream to which the level is written
 * @param level The log level that will be written to the output stream
 * @return The output stream given as the first argument. This allows to chain calls to this operator.
 */
std::ostream& operator<<(std::ostream& out, LoggingLevel level);

} /* namespace n_tools */

#include "forwarddeclare/logger.h"

#endif /* SRC_TOOLS_LOGGER_H_ */
