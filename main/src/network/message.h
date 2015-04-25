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
#include "globallog.h"
#include "archive.h"
#include <sstream>
#include <iosfwd>

namespace n_network {

/**
 * Denote cut-type in gvt synchronization.
 */
enum MessageColor {WHITE=0, RED=1};

std::ostream&
operator<<(std::ostream& os, const MessageColor& c);

/**
 * A Message representing either an event passed between models , or synchronization token
 * between cores.
 */
class Message
{
protected:
	/**
	 * Unique model name of target.
	 */
	const std::string m_destination_model;

	std::size_t m_destination_core;

	std::size_t m_source_core;

	/**
	 * Time message is created (by model/port)
	 */
	t_timestamp m_timestamp;

	/**
	 * Full name of destination port.
	 */
	const std::string m_destination_port;

	/**
	 * Full name of source port.
	 */
	const std::string m_source_port;

	/**
	 * User defined payload.
	 * @see toString()
	 */
	const std::string m_payload;

	/**
	 * Color in synchronization algorithms.
	 * @default WHITE
	 * @see MessageColor
	 */
	MessageColor m_color;

	bool  m_antimessage;

public:
	/**
	 * @param modeldest : destination model
	 * @param time_made
	 * @param destport : full name of destination port
	 * @param sourceport : full name of source port
	 * @param payload : user supplied content.
	 * @note : Color is set by default to white.
	 * @attention Core id is initialized at limits::max().
	 */
	Message(std::string modeldest, const t_timestamp& time_made, std::string destport, std::string sourceport,
	        const std::string& payload = "");

	std::size_t getDestinationCore() const
	{
		return m_destination_core;
	}

	void setAntiMessage(bool b){m_antimessage = b;}

	bool isAntiMessage(){return m_antimessage;}

	void setDestinationCore(std::size_t dest)
	{
		m_destination_core = dest;
	}

	std::size_t getSourceCore() const
	{
		return m_source_core;
	}

	void setSourceCore(std::size_t src)
	{
		m_source_core = src;
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

	virtual std::string toString() const;

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

	MessageColor
	getColor()const {
		return m_color;
	}

	void
	paint(MessageColor newcolor){
		this->m_color = newcolor;
	}

	friend
	bool operator==(const Message& left, const Message& right);

	friend
	bool operator!=(const Message& left, const Message& right);

	/**
	 * Serialize this object to the given archive
	 *
	 * @param archive A container for the desired output stream
	 */
	void serialize(n_serialisation::t_oarchive& archive);

	/**
	 * Unserialize this object to the given archive
	 *
	 * @param archive A container for the desired input stream
	 */
	void serialize(n_serialisation::t_iarchive& archive);

	/**
	 * Helper function for unserializing smart pointers to an object of this class.
	 *
	 * @param archive A container for the desired input stream
	 * @param construct A helper struct for constructing the original object
	 */
	static void load_and_construct(n_serialisation::t_iarchive& archive, cereal::construct<Message>& construct);

};

/**
 * Typedef for client classes
 */
typedef std::shared_ptr<Message> t_msgptr;

/**
 * Comparison object to allow storing msgptrs in min heap queues.
 * @deprecated
 */
struct compare_msgptr{
	bool operator()( const std::shared_ptr<Message>& left, const std::shared_ptr<Message>& right ) const {
		return (left->getTimeStamp() > right->getTimeStamp());
	}
};

} // end namespace n_network



namespace std {
template<>
struct hash<n_network::Message>
{
	/**
	 * Hash specialization for Message. Appends all const members to string, hashes result. (aka poor man's hash)
	 */
	size_t operator()(const n_network::Message& message) const
	{
		std::stringstream ss;
		ss << message.getSourcePort() << message.getDestinationPort() << message.getDestinationModel() << message.getTimeStamp();
		std::string hashkey = ss.str();
		return std::hash<std::string>()(hashkey);
	}
};
}	// end namespace std


#endif /* SRC_NETWORK_MESSAGE_H_ */
