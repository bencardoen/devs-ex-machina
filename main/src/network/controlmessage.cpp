/*
 * controlmessage.cpp
 *
 *  Created on: 10 Apr 2015
 *      Author: ben
 */

#include <controlmessage.h>

namespace n_network {

ControlMessage::ControlMessage(std::size_t cores, t_timestamp tmin, t_timestamp tred)
	: m_tmin(tmin), m_tred(tred), m_gvt_found(false)
{
	m_count = t_count(cores, 0);

}

ControlMessage::~ControlMessage()
{
}

} /* namespace n_network */
