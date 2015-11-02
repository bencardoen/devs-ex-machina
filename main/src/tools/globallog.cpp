/*
 * globallog.cpp
 *
 *  Created on: May 6, 2015
 *      Author: Stijn Manhaeve - Devs Ex Machina
 */

#include "tools/globallog.h"
#ifdef __GNUC__
#include <execinfo.h>
#include <unistd.h>
#endif /* __GNUC__ */

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
	//if gcc, print stacktrace
#ifdef __GNUC__
	/* many thanks to tgamblin and Violet Giraffe for this code
	 * @see http://stackoverflow.com/a/77336
	 */
	void *array[10];
	size_t size;
	// get void*'s for all entries on the stack
	size = backtrace(array, 10);

	// print out all the frames to stderr
	fprintf(stderr, "Error: signal %d:\n", sig);
	backtrace_symbols_fd(array, size, STDERR_FILENO);
#endif /* __GNUC__ */
	restoreHandler();
}
sighandler_t defSigSegv;
sighandler_t defSigTerm;
sighandler_t defSigAbrt;
sighandler_t defSigInt;
sighandler_t defSigFpe;

/**
 * @brief Installs custom signal handlers
 */
int installHandler(){
	defSigSegv = std::signal(SIGSEGV, &signalHandler);
	defSigTerm = std::signal(SIGTERM, &signalHandler);
	defSigAbrt = std::signal(SIGABRT, &signalHandler);
	defSigInt = std::signal(SIGINT, &signalHandler);
	defSigFpe = std::signal(SIGFPE, &signalHandler);

	return 0;
}

void restoreHandler()
{
	std::signal(SIGSEGV, defSigSegv);
	std::signal(SIGTERM, defSigTerm);
	std::signal(SIGABRT, defSigAbrt);
	std::signal(SIGINT, defSigInt);
	std::signal(SIGFPE, defSigFpe);
}

int handlerSet = installHandler();

} /*namespace n_globalLog*/
} /*namespace n_tools*/
#endif
