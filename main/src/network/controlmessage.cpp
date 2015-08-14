/*
 * controlmessage.cpp
 *
 *  Created on: 10 Apr 2015
 *      Author: ben
 */

#include "network/controlmessage.h"

namespace n_network {

ControlMessage::ControlMessage(std::size_t cores, t_timestamp tmin, t_timestamp tred)
	: m_tmin(tmin), m_tred(tred), m_gvt_found(false)
{
	m_count = t_count(cores, 0);
}

ControlMessage::~ControlMessage()
{
}

void ControlMessage::logMessageState()
{
        LOG_DEBUG("Controlmessage current state == ");
        LOG_DEBUG("Current GVT value = ", this->m_gvt, " gvtfound== ", this->m_gvt_found);
        for(size_t i = 0; i<this->m_count.size(); ++i){
                LOG_DEBUG("Count vector @ ", i , " == ", this->m_count[i]);
        }
        LOG_DEBUG("Current tred value = ", this->m_tred);
        LOG_DEBUG("Current tmin value = ", this->m_tmin);
}


} /* namespace n_network */
