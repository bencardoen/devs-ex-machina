/*
 * controlmessage.cpp
 *
 *  Created on: 10 Apr 2015
 *      Author: ben
 */

#include <controlmessage.h>

namespace n_network {

ControlMessage::ControlMessage(std::size_t cores, t_timestamp clock, t_timestamp send):m_clock(clock), m_send(send)
{
	m_count = t_count(cores, 0);

}

ControlMessage::~ControlMessage()
{
}

} /* namespace n_network */
