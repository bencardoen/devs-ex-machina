/*
 * tracemessage.cpp
 *
 *  Created on: Mar 17, 2015
 *      Author: Stijn Manhaeve - Devs Ex Machina
 */

#include <tracers/tracemessage.h>
#include "tools/schedulerfactory.h"
#include "objectfactory.h"

using namespace n_tools;

namespace n_tracers {

TraceMessage::TraceMessage(n_network::t_timestamp time, const t_messagefunc& func)
	: Message("", time), m_func(func)
{
}

void TraceMessage::execute()
{
	m_func();
}

std::shared_ptr<Scheduler<t_tracemessageptr>> scheduler = SchedulerFactory<t_tracemessageptr>::makeScheduler(Storage::FIBONACCI, true);

void scheduleMessage(t_tracemessageptr message)
{
	scheduler->push_back(message);
}

void traceUntil(n_network::t_timestamp time)
{
	std::vector<t_tracemessageptr> messages;
	TraceMessage t(time, nullptr);
	scheduler->unschedule_until(messages, &t);
	for(t_tracemessageptr mess : messages){
		mess->execute();
		n_tools::takeBack(mess);
	}
}

void revertTo(n_network::t_timestamp time)
{
	std::vector<t_tracemessageptr> messages;
	TraceMessage t(time, nullptr);
	scheduler->unschedule_until(messages, &t);
	clearAll();
	for(const t_tracemessageptr& mess: messages)
		scheduler->push_back(mess);
}

void clearAll()
{
	while(!scheduler->empty()){
		t_tracemessageptr ptr = scheduler->pop();
		//TODO unsure whether we have to print all these. I think not
		n_tools::takeBack(ptr);
	}
}

} /* namespace n_tracers */
