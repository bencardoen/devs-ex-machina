/*
 * tracemessage.h
 *
 *  Created on: Mar 17, 2015
 *      Author: Stijn Manhaeve - Devs Ex Machina
 */

#ifndef SRC_TRACERS_TRACEMESSAGE_H_
#define SRC_TRACERS_TRACEMESSAGE_H_

#include <message.h>

namespace n_tracers {

/**
 * @brief Small message for scheduling the tracer output.
 * @note  The code will probably change a lot
 */
class TraceMessage: public n_network::Message
{
public:
	typedef std::function<void()> t_messagefunc;
	/**
	 * @brief Constructor for the TraceMessage
	 * @param time The timestamp of the message.
	 * 		This is the time that the scheduler uses for scheduling the message
	 * @param func A function object. This function will be executed when the message is received.
	 * 		The function must take no arguments and return no result.
	 */
	TraceMessage(n_network::t_timestamp time, const t_messagefunc& func);

	/**
	 * @brief Executes the scheduled functionality.
	 */
	void execute();

private:
	t_messagefunc m_func;	//function to be executed. This function takes no arguments
};

} /* namespace n_tracers */

#endif /* SRC_TRACERS_TRACEMESSAGE_H_ */
