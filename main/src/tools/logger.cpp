/*
 * logger.cpp
 *
 *  Created on: Mar 14, 2015
 *      Author: Stijn Manhaeve - Devs Ex Machina
 */

#include "tools/logger.h"

namespace n_tools {

	std::ostream& operator <<(std::ostream& out, LoggingLevel level) {
		switch(level){
			case E_DEBUG:
				out << "DEBUG";
				break;
			case E_ERROR:
				out << "ERROR";
				break;
			case E_INFO:
				out << "INFO";
				break;
			case E_WARNING:
				out << "WARNING";
				break;
			default:
				out << "USER DEFINED";
				break;
		}
		return out;
	}

} /* namespace n_tools */
