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
	/**
	 * Unique model name of target.
	 */
	const std::string m_destination_model;

	/**
	 * Core id in this simulator.
	 */
	std::size_t m_destination_core;

	/**
	 * Time message is created (by model/port)
	 */
	const t_timestamp m_timestamp;

	/**
	 * Full name of destination port.
	 */
	const std::string m_destination_port;//fullname

	/**
	 * Full name of source port.
	 */
	const std::string m_source_port;	// fullname

public:
	Message(std::string modeldest, const t_timestamp& time_made, std::string destport, std::string sourceport)
		: m_destination_model(modeldest), m_destination_core(std::numeric_limits<std::size_t>::max()), m_timestamp(time_made), m_destination_port(destport),m_source_port(sourceport)
	{
	}

	std::size_t getDestinationCore() const
	{
		return m_destination_core;
	}

	std::string getDestinationPort()const
	{
		return m_destination_port;
	}

	std::string getDestinationModel() const
	{
		return m_destination_model;
	}

	std::string getSourcePort()const
	{
		return m_source_port;
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
