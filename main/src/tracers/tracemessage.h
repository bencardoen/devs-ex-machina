/*
 * tracemessage.h
 *
 *  Created on: Mar 17, 2015
 *      Author: Stijn Manhaeve - Devs Ex Machina
 */

#ifndef SRC_TRACERS_TRACEMESSAGE_H_
#define SRC_TRACERS_TRACEMESSAGE_H_

#include <functional>	//defines std::less<T>
#include <limits>	//defines std::numeric_limits<T>
#include "message.h"

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
	 * @param takeback [optional, default value: []{}, the empty lambda function] A cleanup function object.
	 * 		This function is called when the message is destroyed, in order to clean up any memory that can otherwise no longer be accessed
	 * @precondition The two function objects are valid function pointers. No nullptr allowed!
	 */
	TraceMessage(n_network::t_timestamp time,
		const t_messagefunc& func,
		std::size_t coreID,
	        const t_messagefunc& takeback = [] {});


	~TraceMessage();

	/**
	 * @brief Executes the scheduled functionality.
	 */
	void execute();

	/**
	 * @brief Comparison operator needed for scheduling these messages in a scheduler
	 */
	bool operator<(const TraceMessage& other) const;

	/**
	 * @brief Comparison operator needed for scheduling these messages in a scheduler
	 */
	bool operator>(const TraceMessage& other) const;

private:
	t_messagefunc m_func;		//function to be executed. This function takes no arguments
	t_messagefunc m_takeBack;	//function for destroying this object
};

typedef TraceMessage* t_tracemessageptr;

/**
 * Schedules a trace message.
 * @param message A pointer to the message that must be scheduled.
 * @precodition message is a valid message
 */
void scheduleMessage(t_tracemessageptr message);
/**
 * Performs all output
 * @param time. All output scheduled before this time will be printed
 * @warning Writing the output itself is asynchronously.
 * 	 Make sure that, before exiting the program, this thread has been stopped.
 * 	 Normally, the Tracers class handles this for you, but if you are feeling adventurous, don't forget this function!
 * @see waitForTracer
 */
void traceUntil(n_network::t_timestamp time);
/**
 * @brief reverts the output of a single core to a certain time.
 * @param coreID [default -1] Only throw away trace messages with this ID. If -1, throw away everything
 */
void revertTo(n_network::t_timestamp time, std::size_t coreID = std::numeric_limits<std::size_t>::max());
/**
 * @brief clears the entire queue of trace messages.
 */
void clearAll();
/**
 * @brief block current thread until the previous batch of trace messages has been dealt with.
 * Normally, the Tracers class takes care of this.
 * However, if you are messing around with trace messages yourself, do not forget calling this method!
 */
void waitForTracer();

/**
 * Entry for a TraceMessage in a scheduler.
 * Provides overloads for the most important comparison operators.
 * @attention : reverse ordered on time : 1 > 2 == true (for max heap).
 */
class TraceMessageEntry
{
	t_tracemessageptr m_pointer;
public:
	/**
	 * @brief Gives access to the raw pointer contained within this entry
	 */
	t_tracemessageptr getPointer() const;

	TraceMessageEntry(const t_tracemessageptr& ptr);
	TraceMessageEntry(const TraceMessageEntry&) = default;
	TraceMessageEntry(TraceMessageEntry&&) = default;
	TraceMessageEntry& operator=(const TraceMessageEntry&) = default;
	TraceMessageEntry& operator=(TraceMessageEntry&&) = default;

	TraceMessage& operator*();
	const TraceMessage& operator*() const;
	TraceMessage* operator->();
	const TraceMessage* operator->() const;

	friend
	bool operator<(const TraceMessageEntry& lhs, const TraceMessageEntry& rhs);

	friend
	bool operator>(const TraceMessageEntry& lhs, const TraceMessageEntry& rhs);

	friend
	bool operator>=(const TraceMessageEntry& lhs, const TraceMessageEntry& rhs);

	friend
	bool operator==(const TraceMessageEntry& lhs, const TraceMessageEntry& rhs);

	friend std::ostream& operator<<(std::ostream& os, const TraceMessageEntry& rhs);
};

} /* namespace n_tracers */

namespace std {
template<>
struct hash<n_tracers::TraceMessageEntry>
{
	size_t operator()(const n_tracers::TraceMessageEntry& item) const
	{
		//just hash on the pointer value
		return hash<n_tracers::t_tracemessageptr>()(item.getPointer());
	}
};
}

#endif /* SRC_TRACERS_TRACEMESSAGE_H_ */
