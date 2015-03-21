/*
 * tracemessage.cpp
 *
 *  Created on: Mar 17, 2015
 *      Author: Stijn Manhaeve - Devs Ex Machina
 */

#include <tracers/tracemessage.h>

namespace n_tracers {

TraceMessage::TraceMessage(n_network::t_timestamp time, const t_messagefunc& func)
	: Message("", time), m_func(func)
{
}

void TraceMessage::execute()
{
	m_func();
}

} /* namespace n_tracers */

