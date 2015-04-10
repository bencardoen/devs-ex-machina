/*
 * controlmessage.h
 *
 *  Created on: 10 Apr 2015
 *      Author: Ben Cardoen
 */

#ifndef SRC_NETWORK_CONTROLMESSAGE_H_
#define SRC_NETWORK_CONTROLMESSAGE_H_

#include "timestamp.h"
#include <vector>
#include <memory>

typedef std::vector<int> t_count;

namespace n_network {
/**
 * Controlmessage, passed in 1-2 rounds during Mattern's algorithm.
 */
class ControlMessage
{
private:
	t_timestamp		m_clock;
	t_timestamp		m_send;
	t_count			m_count;
public:
	ControlMessage(size_t cores, t_timestamp clock, t_timestamp send);
	virtual ~ControlMessage();
	t_count&	getCountVector(){return m_count;}
};

typedef std::shared_ptr<ControlMessage> t_controlmsg;

} /* namespace n_network */

#endif /* SRC_NETWORK_CONTROLMESSAGE_H_ */
