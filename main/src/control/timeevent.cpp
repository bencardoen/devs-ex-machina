/*
 * timeevent.cpp
 *
 *  Created on: 11 May 2015
 *      Author: matthijs
 */

#include "control/timeevent.h"

namespace n_control {

TimeEvent::TimeEvent(t_timestamp t, size_t d, bool r)
	: m_type(PAUSE), m_time(t), m_interval(t), m_duration(d), m_repeating(r)
{
}

TimeEvent::TimeEvent(t_timestamp t, std::string pf, bool r)
	: m_type(SAVE), m_time(t), m_interval(t), m_duration(0), m_prefix(pf), m_repeating(r)
{
}

bool TimeEvent::operator<(const TimeEvent& rhs) const
{
	return m_time < rhs.m_time; // Sort highest time at the front
}

void TimeEvent::advance()
{
	if (m_repeating)
		m_time = m_time + m_interval;
}

void TimeEventQueue::push(TimeEvent te)
{
	m_queue.push_back(te);
	std::sort(m_queue.begin(), m_queue.end());
}

int TimeEventQueue::countTodo(const t_timestamp& now) const
{
	int amnt = 0;
	for (const TimeEvent& te : m_queue) {
		if (te.m_time > now)
			break;
		++amnt;
	}
	return amnt;
}

std::vector<TimeEvent> TimeEventQueue::popUntil(const t_timestamp& now)
{
	std::vector<TimeEvent> worklist;
	while (!m_queue.empty()) {
		TimeEvent ev = m_queue.back(); // Take the earliest element
		if (ev.m_time > now)		// If this is not pending, it's no use looking any further
			break;
		worklist.push_back(ev);
		m_queue.pop_back();

		if (ev.m_repeating) {
			ev.advance();
			push(ev);
		}
	}
	return worklist;
}

bool TimeEventQueue::todo(const t_timestamp& now) const
{
	return (!m_queue.empty() && m_queue.back().m_time <= now);
}

} /* namespace n_control */

