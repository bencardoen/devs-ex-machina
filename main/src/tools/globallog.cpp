/*
 * globallog.cpp
 *
 *  Created on: May 6, 2015
 *      Author: Stijn Manhaeve - Devs Ex Machina
 */

#include "globallog.h"

#if LOGGING==true

namespace n_tools {
namespace n_globalLog {

/**
 * @brief Custom signal handler for when stuff goes wrong
 * The handler will (hopefully) log a message and flush the logger
 */
void signalHandler(int sig){
	LOG_ERROR("Critical failure: aborted with signal ", sig);
	LOG_FLUSH;
}

/**
 * @brief Installs custom signal handlers
 */
int installHandler(){
	std::signal(SIGSEGV, &signalHandler);
	std::signal(SIGTERM, &signalHandler);
	std::signal(SIGABRT, &signalHandler);

	return 0;
}

int handlerSet = installHandler();

} /*namespace n_globalLog*/
} /*namespace n_tools*/
#endif
