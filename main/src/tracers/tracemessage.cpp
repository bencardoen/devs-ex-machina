/*
 * tracemessage.cpp
 *
 *  Created on: Mar 17, 2015
 *      Author: Stijn Manhaeve - Devs Ex Machina
 */

#include "tracemessage.h"
#include "schedulerfactory.h"
#include "objectfactory.h"

using namespace n_tools;

namespace n_tracers {

TraceMessage::TraceMessage(n_network::t_timestamp time, const t_messagefunc& func, std::size_t tracerID)
	: Message("", time, "", ""), m_func(func), m_tracerID(tracerID)
{
}

void TraceMessage::execute()
{
	m_func();
}

bool TraceMessage::operator <(const TraceMessage& other) const
{
	if (this->getTimeStamp() == other.getTimeStamp())
		return this->m_tracerID < other.m_tracerID;
	return (this->getTimeStamp() < other.getTimeStamp());
}

bool TraceMessage::operator >(const TraceMessage& other) const
{
	if (this->getTimeStamp() == other.getTimeStamp())
		return this->m_tracerID > other.m_tracerID;
	return (this->getTimeStamp() > other.getTimeStamp());
}

std::shared_ptr<Scheduler<TraceMessageEntry>> scheduler = SchedulerFactory<TraceMessageEntry>::makeScheduler(
        Storage::FIBONACCI, true);

void scheduleMessage(t_tracemessageptr message)
{
	scheduler->push_back(TraceMessageEntry(message));
}

void traceUntil(n_network::t_timestamp time)
{
	std::vector<TraceMessageEntry> messages;
	TraceMessage t(time, nullptr, std::numeric_limits<std::size_t>::max());
	scheduler->unschedule_until(messages, &t);
	for (TraceMessageEntry& mess : messages) {
		LOG_DEBUG("TRACE: executing trace message at time.", mess->getTimeStamp());
		mess->execute();
		n_tools::takeBack(mess.getPointer());
	}
}

void revertTo(n_network::t_timestamp time)
{
	std::vector<TraceMessageEntry> messages;
	TraceMessage t(time, nullptr, std::numeric_limits<std::size_t>::max());
	scheduler->unschedule_until(messages, &t);
	clearAll();
	for (const TraceMessageEntry& mess : messages)
		scheduler->push_back(mess);
}

void clearAll()
{
	while (!scheduler->empty()) {
		TraceMessageEntry ptr = scheduler->pop();
		//TODO unsure whether we have to print all these. I think not
		n_tools::takeBack(ptr.getPointer());
	}
}

} /* namespace n_tracers */
