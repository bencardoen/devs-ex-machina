/*
 * logger.cpp
 *
 *  Created on: Mar 14, 2015
 *      Author: Stijn Manhaeve - Devs Ex Machina
 */

#include <logger.h>

namespace n_tools {
	Logger::Logger(const char* filename, int level) :m_buf(new async_buf(filename)), m_out(m_buf), m_levelFilter(level), m_doPrint(false){
		m_out.rdbuf(m_buf);
	}
	Logger::~Logger() {
		m_out.flush();
		delete m_buf;
	}

	void Logger::startEntry(LoggingLevel level) {
		if(m_doPrint) m_out << '\n';
		if(level & m_levelFilter) {
			m_doPrint = true;
			m_out << level << ": ";
		}
	}

	std::ostream& operator <<(std::ostream& out, Logger::LoggingLevel level) {
		switch(level){
			case Logger::E_DEBUG:
				out << "DEBUG";
				break;
			case Logger::E_ERROR:
				out << "ERROR";
				break;
			case Logger::E_INFO:
				out << "INFO";
				break;
			case Logger::E_WARNING:
				out << "WARNING";
				break;
			default:
				out << "USER DEFINED";
				break;
		}
		return out;
	}

} /* namespace n_tools */
