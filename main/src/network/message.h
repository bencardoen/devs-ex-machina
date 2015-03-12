/*
 * message.h
 *
 *  Created on: 12 Mar 2015
 *      Author: Ben Cardoen
 */

#ifndef SRC_NETWORK_MESSAGE_H_
#define SRC_NETWORK_MESSAGE_H_

#include "timestamp.h"

namespace n_network {

/**
 * A Message representing either an event passed between models , or synchronization token
 * between cores.
 */
class Message
{
private:
	const std::string m_destination_model;
	std::size_t m_destination_core;
	const t_timestamp m_timestamp;

public:
	Message(std::string modeldest, std::size_t coredest, const t_timestamp& time_made)
		: m_destination_model(modeldest), m_destination_core(coredest), m_timestamp(time_made)
	{
		;
	}

	std::size_t getDestinationCore() const
	{
		return m_destination_core;
	}

	void setDestinationCore(std::size_t dest)
	{
		m_destination_core = dest;
	}

	virtual ~Message()
	{
		;
	}
};

typedef std::shared_ptr<Message> t_msgptr;

} // end namespace

#endif /* SRC_NETWORK_MESSAGE_H_ */
