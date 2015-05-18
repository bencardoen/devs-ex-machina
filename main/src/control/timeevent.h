/*
 * timeevent.h
 *
 *  Created on: 11 May 2015
 *      Author: matthijs
 */

#ifndef SRC_CONTROL_TIMEEVENT_H_
#define SRC_CONTROL_TIMEEVENT_H_

#include "timestamp.h"

namespace n_control {

using namespace n_network;

/**
 * @brief An event occurring at some time during the simulation.
 * This can be either a request to save everything, or to pause for a while
 */
struct TimeEvent
{
	/**
	 * The type of TimeEvent
	 */
	enum Type
	{
		PAUSE,	///< Pause the simulation
		SAVE,	///< Save the simulation
	} m_type;

	/*
	 * Time at which the event occurs (next)
	 */
	t_timestamp m_time;

	/*
	 * For repeating events: rate at which an event occurs.
	 * Is set to the same value as m_time in constructor
	 */
	t_timestamp m_interval;

	/*
	 * For PAUSE events: duration of the pause
	 */
	size_t m_duration;

	/*
	 * For SAVE events: prefix of file to be saved
	 */
	std::string m_prefix;

	/*
	 * Whether the event repeats every <m_time> times
	 */
	bool m_repeating;

	/**
	 * @brief Ctor for PAUSE TimeEvents
	 */
	TimeEvent(t_timestamp t, size_t d, bool r);

	/**
	 * @brief Ctor for SAVE TimeEvents
	 */
	TimeEvent(t_timestamp t, std::string pf, bool r);

	/**
	 * @brief Operator which sorts TimeEvents in ASCENDING m_time order
	 */
	bool operator<(const TimeEvent& rhs) const;

	/**
	 * @brief If this event is repeating, push it one interval ahead
	 */
	void advance();
};

class TimeEventQueue
{
private:
	std::vector<TimeEvent> m_queue;
public:
	TimeEventQueue() {};
	~TimeEventQueue() {};

	void push(TimeEvent te);

	/**
	 * @brief sort the event queue
	 */
	void prepare();

	/**
	 * @brief Checks if any still unhandled events are pending
	 */
	bool todo(const t_timestamp& now) const;

	/**
	 * @brief Count how many unhandled events are waiting in the queue
	 */
	int countTodo(const t_timestamp& now) const;

	/**
	 * @brief Pops and returns any unhandled time events from the queue
	 */
	std::vector<TimeEvent> popUntil(const t_timestamp& now);
};

} /* namespace n_control */

#endif /* SRC_CONTROL_TIMEEVENT_H_ */
