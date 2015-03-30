/*
 * message.h
 *
 *  Created on: 12 Mar 2015
 *      Author: Ben Cardoen
 */

#ifndef SRC_NETWORK_MESSAGE_H_
#define SRC_NETWORK_MESSAGE_H_

#include "timestamp.h"
#include "stringtools.h"

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
	t_timestamp m_timestamp;

	/**
	 * Full name of destination port.
	 */
	const std::string m_destination_port; //fullname

	/**
	 * Full name of source port.
	 */
	const std::string m_source_port;	// fullname

	/**
	 * User defined payload.
	 * @see toString()
	 */
	const std::string m_payload;

public:
	/**
	 * @param modeldest : destination model
	 * @param time_made
	 * @param destport : full name of destination port
	 * @param sourceport : full name of source port
	 * @param payload : user supplied content.
	 * @attention Core id is initialized at limits::max().
	 */
	Message(std::string modeldest, const t_timestamp& time_made, std::string destport, std::string sourceport,
	        const std::string& payload = "")
		:
		m_destination_model(modeldest), m_destination_core(std::numeric_limits<std::size_t>::max()),
		m_timestamp(time_made), m_destination_port(destport), m_source_port(sourceport), m_payload(payload)
	{
	}

	std::size_t getDestinationCore() const
	{
		return m_destination_core;
	}

	std::string getDestinationPort() const
	{
		return n_tools::copyString(m_destination_port);
	}

	std::string getDestinationModel() const
	{
		return n_tools::copyString(m_destination_model);
	}

	std::string getSourcePort() const
	{
		return n_tools::copyString(m_source_port);
	}

	std::string getPayload() const
	{
		return n_tools::copyString(m_payload);
	}

	void setDestinationCore(std::size_t dest)
	{
		m_destination_core = dest;
	}

	virtual std::string toString() const
	{
		std::stringstream result;
		result << "Message from " << this->getSourcePort() << " to " << this->getDestinationPort();
		result << " @" << m_timestamp;
		result << " to model " << this->getDestinationModel() << " @core_nr " << m_destination_core;
		result << " payload " << this->getPayload();
		return result.str();
	}

	t_timestamp getTimeStamp() const
	{
		return m_timestamp;
	}

	void setTimeStamp(const t_timestamp& now)
	{
		m_timestamp = now;
	}

	virtual ~Message()
	{
		;
	}
};

typedef std::shared_ptr<Message> t_msgptr;

} // end namespace

#endif /* SRC_NETWORK_MESSAGE_H_ */
