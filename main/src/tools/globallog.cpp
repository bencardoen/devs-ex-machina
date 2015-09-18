/*
 * globallog.cpp
 *
 *  Created on: May 6, 2015
 *      Author: Stijn Manhaeve - Devs Ex Machina
 */

#include "tools/globallog.h"

#if LOGGING==true

namespace n_tools {
namespace n_globalLog {

const char* sig2str(int sig){
	switch(sig){
	case SIGTERM:
		return "SIGTERM";
	case SIGSEGV:
		return "SIGSEGV";
	case SIGINT:
		return "SIGINT";
	case SIGILL:
		return "SIGILL";
	case SIGABRT:
		return "SIGABRT";
	case SIGFPE:
		return "SIGFPE";
	default:
		return "unknown signal";
	}
}
void restoreHandler();

/**
 * @brief Custom signal handler for when stuff goes wrong
 * The handler will (hopefully) log a message and flush the logger
 */
void signalHandler(int sig){
	LOG_ERROR("Critical failure: aborted with signal ", sig, " (", sig2str(sig), ')');
	LOG_FLUSH;
	if(sig == SIGINT)
		std::terminate();
	restoreHandler();
}
sighandler_t defSigSegv;
sighandler_t defSigTerm;
sighandler_t defSigAbrt;
sighandler_t defSigInt;

/**
 * @brief Installs custom signal handlers
 */
int installHandler(){
	defSigSegv = std::signal(SIGSEGV, &signalHandler);
	defSigTerm = std::signal(SIGTERM, &signalHandler);
	defSigAbrt = std::signal(SIGABRT, &signalHandler);
	defSigInt = std::signal(SIGINT, &signalHandler);

	return 0;
}

void restoreHandler()
{
	std::signal(SIGSEGV, defSigSegv);
	std::signal(SIGTERM, defSigTerm);
	std::signal(SIGABRT, defSigAbrt);
	std::signal(SIGINT, defSigInt);
}

int handlerSet = installHandler();

} /*namespace n_globalLog*/
} /*namespace n_tools*/
#endif
