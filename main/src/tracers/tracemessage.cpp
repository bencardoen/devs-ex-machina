/*
 * tracemessage.cpp
 *
 *  Created on: Mar 17, 2015
 *      Author: Stijn Manhaeve - Devs Ex Machina
 */

#include "tracers/tracemessage.h"
#include "tools/schedulerfactory.h"
#include "tools/objectfactory.h"
#include "tools/globallog.h"
#include <thread>
#include <future>

using namespace n_tools;

namespace n_tracers {

TraceMessage::TraceMessage(n_network::t_timestamp time, const t_messagefunc& func, std::size_t coreID, const t_messagefunc& takeback)
	: m_time(time.getTime(), time.getCausality()+1), m_coreid(coreID), m_func(func), m_takeBack(takeback)
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
	if (this->getTime() == other.getTime()){
		return this < &other;
	}
	return (this->getTime() < other.getTime());
}

bool TraceMessage::operator >(const TraceMessage& other) const
{
	if (this->getTime() == other.getTime()){
		return this > &other;
	}
	return (this->getTime() > other.getTime());
}

t_tracemessageptr TraceMessageEntry::getPointer() const
{
	return m_pointer;
}

TraceMessageEntry::TraceMessageEntry(const t_tracemessageptr& ptr)
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

bool operator<=(const TraceMessageEntry& lhs, const TraceMessageEntry& rhs)
{
	return (!(lhs > rhs));
}

bool operator==(const TraceMessageEntry& lhs, const TraceMessageEntry& rhs)
{
	return (lhs.m_pointer == rhs.m_pointer);
}

std::ostream& operator<<(std::ostream& os, const TraceMessageEntry& rhs)
{
	return (os << "Trace message scheduled at " << rhs->getTime());
}

#ifdef NO_TRACER

void scheduleMessage(t_tracemessageptr message)
{
}

void waitForTracer()
{
}

void doRealTrace(std::vector<TraceMessageEntry>){
}

void traceUntil(n_network::t_timestamp)
{
}

void revertTo(n_network::t_timestamp, std::size_t)
{
}

void clearAll()
{
}

#else /* USE_TRACER */

std::shared_ptr<Scheduler<TraceMessageEntry>> scheduler = SchedulerFactory<TraceMessageEntry>::makeScheduler(
        Storage::FIBONACCI, true);
std::atomic<std::future<void>*> tracerFuture(nullptr);
std::mutex mu;

void scheduleMessage(t_tracemessageptr message)
{
	assert(message && "scheduleMessage: Can't schedule a nullptr message");
	scheduler->push_back(TraceMessageEntry(message));
}

void waitForTracer()
{
	std::lock_guard<std::mutex> guard(mu);
	if (tracerFuture.load() != nullptr) {
		tracerFuture.load()->wait();
		delete tracerFuture.load();
		tracerFuture.store(nullptr);
	}
}

void doRealTrace(std::vector<TraceMessageEntry> entries){
	for (TraceMessageEntry& mess : entries) {
		LOG_DEBUG("TRACE: executing trace message at time.", mess->getTime());
		mess->execute();
		n_tools::takeBack(mess.getPointer());
	}
}

void traceUntil(n_network::t_timestamp time)
{
	std::lock_guard<std::mutex> guard(mu);
	std::vector<TraceMessageEntry> messages;
	TraceMessage t(time, [] {}, 0u);
	scheduler->unschedule_until(messages, &t);
	if(messages.empty())
		return;

	if (tracerFuture.load() != nullptr) {
		tracerFuture.load()->wait();
		delete tracerFuture.load();
		tracerFuture.store(nullptr);
	}

	tracerFuture.store(new std::future<void>(std::async(std::launch::async, std::bind(&doRealTrace, messages))));
}

void revertTo(n_network::t_timestamp time, std::size_t coreID)
{
	std::vector<TraceMessageEntry> messages;
#ifdef SAFETY_CHECKS
        if(isZero(time)){
                LOG_ERROR("unsigned 0-1, time arg = ", time, " id = ", coreID);
                throw std::out_of_range("Revert with 0 time");
                LOG_FLUSH;
        }
#endif
	TraceMessage t(time.getTime()-n_network::t_timestamp::epsilon().getTime(), [] {}, 0u);
        
	scheduler->unschedule_until(messages, &t);
	std::vector<TraceMessageEntry> messagesLost;
	TraceMessage inf(n_network::t_timestamp::infinity(), [] {}, 0u);
	scheduler->unschedule_until(messagesLost, &inf);
	LOG_DEBUG("revertTo: reverting back messages to time ", time, " from core ", coreID, " total of ",
	        messagesLost.size(), " messages");
	if (coreID == std::numeric_limits<std::size_t>::max()) {
		LOG_DEBUG("revertTo: dumping all messages until time ", time, " total of ", messagesLost.size(),
		        " messages");
		for (const TraceMessageEntry& mess : messagesLost)
			n_tools::takeBack(mess.getPointer());
	} else {
		for (const TraceMessageEntry& mess : messagesLost) {
			if (mess->getCoreID() != coreID)
				scheduler->push_back(mess);
			else
				n_tools::takeBack(mess.getPointer());
		}
	}
	for (const TraceMessageEntry& mess : messages)
		scheduler->push_back(mess);
}

void clearAll()
{
	while (!scheduler->empty()) {
		TraceMessageEntry ptr = scheduler->pop();
		n_tools::takeBack(ptr.getPointer());
	}
}

#endif /* USE_TRACER */

} /* namespace n_tracers */
