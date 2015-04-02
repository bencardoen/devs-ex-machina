/*
 * tracemessage.cpp
 *
 *  Created on: Mar 17, 2015
 *      Author: Stijn Manhaeve - Devs Ex Machina
 */

#include "tracemessage.h"
#include "schedulerfactory.h"
#include "objectfactory.h"
#include "globallog.h"
#include <thread>
#include <future>

using namespace n_tools;

namespace n_tracers {

TraceMessage::TraceMessage(n_network::t_timestamp time, std::size_t tracerID, const t_messagefunc& func, const t_messagefunc& takeback)
	: Message("", time, "", ""), m_func(func), m_takeBack(takeback), m_tracerID(tracerID)
{
	assert(m_func != nullptr && "TraceMessage::TraceMessage can't accept nullptr as execution function");
	assert(m_takeBack != nullptr && "TraceMessage::TraceMessage Can't accept nullptr as cleanup function. If you don't need a cleanup function, either provide an empty one or omit the argument.");
}

TraceMessage::~TraceMessage()
{
	m_takeBack();
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

t_tracemessageptr TraceMessageEntry::getPointer() const
{
	return m_pointer;
}

TraceMessageEntry::TraceMessageEntry(t_tracemessageptr ptr)
	: m_pointer(ptr)
{
}
TraceMessage& TraceMessageEntry::operator*()
{
	return *m_pointer;
}
const TraceMessage& TraceMessageEntry::operator*() const
{
	return *m_pointer;
}
TraceMessage* TraceMessageEntry::operator->()
{
	return m_pointer;
}
const TraceMessage* TraceMessageEntry::operator->() const
{
	return m_pointer;
}

bool operator<(const TraceMessageEntry& lhs, const TraceMessageEntry& rhs)
{
	if (!(*lhs < *rhs) && !(*lhs > *rhs)) {
		LOG_DEBUG("TRACER: timestamps are equal: ", lhs->getTimeStamp(), " == ", rhs->getTimeStamp());
		return lhs.m_pointer > rhs.m_pointer;
	}
	return *lhs > *rhs;
}

bool operator>(const TraceMessageEntry& lhs, const TraceMessageEntry& rhs)
{
	return (rhs > lhs);
}

bool operator>=(const TraceMessageEntry& lhs, const TraceMessageEntry& rhs)
{
	return (!(lhs < rhs));
}

bool operator==(const TraceMessageEntry& lhs, const TraceMessageEntry& rhs)
{
	return (lhs.m_pointer == rhs.m_pointer);
}

std::ostream& operator<<(std::ostream& os, const TraceMessageEntry& rhs)
{
	return (os << "Trace message scheduled at " << rhs->getTimeStamp());
}


std::shared_ptr<Scheduler<TraceMessageEntry>> scheduler = SchedulerFactory<TraceMessageEntry>::makeScheduler(
        Storage::FIBONACCI, true);
std::future<void>* tracerFuture = nullptr;

void scheduleMessage(t_tracemessageptr message)
{
	assert(message && "scheduleMessage: Can't schedule a nullptr message");
	scheduler->push_back(TraceMessageEntry(message));
}

void waitForTracer()
{
	if(tracerFuture != nullptr){
		tracerFuture->wait();
		delete tracerFuture;
		tracerFuture = nullptr;
	}
}

void doRealTrace(std::vector<TraceMessageEntry> entries){
	for (TraceMessageEntry& mess : entries) {
		LOG_DEBUG("TRACE: executing trace message at time.", mess->getTimeStamp());
		mess->execute();
		n_tools::takeBack(mess.getPointer());
	}
}

void traceUntil(n_network::t_timestamp time)
{
	std::vector<TraceMessageEntry> messages;
	TraceMessage t(time, std::numeric_limits<std::size_t>::max(), []{});
	scheduler->unschedule_until(messages, &t);
	waitForTracer();
	tracerFuture = new std::future<void>(std::async(std::launch::async, std::bind(&doRealTrace, messages)));
}

void revertTo(n_network::t_timestamp time)
{
	std::vector<TraceMessageEntry> messages;
	TraceMessage t(time, std::numeric_limits<std::size_t>::max(), []{});
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
