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

	class Logger {
		public:
	    		enum LoggingLevel{
	    			E_ERROR = 1,
					E_WARNING = 2,
					E_DEBUG = 4,
	    			E_INFO = 8
	    		};
	    	Logger(const char* filename, int level = E_ERROR | E_WARNING | E_DEBUG | E_INFO);
	    	~Logger();

	    	void startEntry(LoggingLevel level);

	    	template<typename T>
	    	Logger& operator<<(const T& data){
	    		std::lock_guard<std::mutex> m(this->m_mutex);
#ifndef LOG_USEGLOBAL
	    		if(m_doPrint)
#endif
	    			m_out << data;
	    		return *this;
	    	}

		private:
	    	ASynchWriter* m_buf;
	    	std::ostream m_out;
	    	int m_levelFilter;
	    	bool m_doPrint;
	    	std::mutex m_mutex;

	};

	std::ostream& operator<<(std::ostream& out, Logger::LoggingLevel level);


} /* namespace n_tools */

//you can define the log level here. if you want to
#define LOG_LEVEL 8

#endif /* SRC_TOOLS_LOGGER_H_ */
