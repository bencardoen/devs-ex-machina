/*
 * tracemessage.h
 *
 *  Created on: Mar 17, 2015
 *      Author: Stijn Manhaeve - Devs Ex Machina
 */

#ifndef SRC_TRACERS_TRACEMESSAGE_H_
#define SRC_TRACERS_TRACEMESSAGE_H_

#include <functional>	//defines std::less<T>
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

typedef TraceMessage* t_tracemessageptr;

void scheduleMessage(t_tracemessageptr message);
void traceUntil(n_network::t_timestamp time);
void revertTo(n_network::t_timestamp time);
void clearAll();

} /* namespace n_tracers */

namespace std {
template<>
struct less<n_tracers::t_tracemessageptr>
{
	bool operator()(const n_tracers::t_tracemessageptr& k1, const n_tracers::t_tracemessageptr& k2) const
	{
		//TODO get the timestamp from the messages and compare those
		return k1 < k2;
	}
};
}

#endif /* SRC_TRACERS_TRACEMESSAGE_H_ */
