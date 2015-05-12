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
	};

	TimeEvent(t_timestamp t, Type et, bool r, size_t d = 0)
		: m_time(t), m_type(et), m_duration(d), m_repeating(r)
	{
	}
	;

	t_timestamp m_time;
	Type m_type;
	size_t m_duration;

	/*
	 * Whether the event repeats every <m_time> times
	 */
	bool m_repeating;
	bool operator<(const TimeEvent& rhs) const
	{
		return m_time > rhs.m_time; // Sort lowest time first
	}
};

class TimeEventQueue
{
private:
	std::vector<TimeEvent> m_queue;
public:
	TimeEventQueue() {};
	~TimeEventQueue() {};

	void push(TimeEvent te) {
		m_queue.push_back(te);
	}

	void prepare() {
		std::sort(m_queue.begin(), m_queue.end());
	}
	/**
	 * @brief Count how many unhandled events are waiting in the queue
	 */
	int countTodo(const t_timestamp& now)
	{
		int amnt = 0;
		for(TimeEvent& te : m_queue) {
			if(te.m_time > now) break;
			++amnt;
		}
		return amnt;
	}

	/**
	 * @brief Pops and returns any unhandled time events from the queue
	 */
	std::vector<TimeEvent> popUntil(const t_timestamp& now)
	{
		std::vector<TimeEvent> worklist;
		while (!m_queue.empty()) {
			TimeEvent& ev = m_queue.front();
			if(ev.m_time < now) break;
			worklist.push_back(ev);
			m_queue.pop_back();
		}
		return worklist;
	}
};

} /* namespace n_control */

#endif /* SRC_CONTROL_TIMEEVENT_H_ */
