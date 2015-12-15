/*
 * This file is part of the DEVS Ex Machina project.
 * Copyright 2014 - 2015 University of Antwerp
 * https://www.uantwerpen.be/en/
 * Licensed under the EUPL V.1.1
 * A full copy of the license is in COPYING.txt, or can be found at
 * https://joinup.ec.europa.eu/community/eupl/og_page/eupl
 *      Author: Stijn Manhaeve, Tim Tuijn
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
