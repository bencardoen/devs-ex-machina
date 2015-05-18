/*
 * message.h
 *
 *  Created on: 12 Mar 2015
 *      Author: Ben Cardoen
 */

#ifndef SRC_NETWORK_MESSAGE_H_
#define SRC_NETWORK_MESSAGE_H_

#include <serialization/archive.h>
#include "timestamp.h"
#include "stringtools.h"
#include "globallog.h"
#include <sstream>
#include <iosfwd>

namespace n_network {

/**
 * Denote cut-type in gvt synchronization.
 */
enum MessageColor
{
	WHITE = 0, RED = 1
};

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

	/**
	 * Is message an annihilator of the original ?
	 */
	bool m_antimessage;

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

	void setAntiMessage(bool b)
	{
		m_antimessage = b;
	}

	bool isAntiMessage() const
	{
		return m_antimessage;
	}

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

	virtual std::string getPayload() const
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

	MessageColor getColor() const
	{
		return m_color;
	}

	void paint(MessageColor newcolor)
	{
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
	void serialize(n_serialization::t_oarchive& archive);


	/**
	 * Helper function for unserializing smart pointers to an object of this class.
	 *
	 * @param archive A container for the desired input stream
	 * @param construct A helper struct for constructing the original object
	 */
	static void load_and_construct(n_serialization::t_iarchive& archive, cereal::construct<Message>& construct);

};

/**
 * Typedef for client classes
 */
typedef std::shared_ptr<Message> t_msgptr;

/**
 * @brief Message class for sending data that is not a string.
 * @note
 */
template<typename DataType>
class SpecializedMessage: public Message
{
public:
	SpecializedMessage(std::string modeldest, const t_timestamp& time_made, std::string destport, std::string sourceport, const DataType& data):
		Message(modeldest, time_made, destport, sourceport, std::string(reinterpret_cast<const char*>(&(data)), sizeof(DataType)))
	{
		static_assert(!std::is_pointer<DataType>::value,
			"Using pointer types is not allowed.\n"
			" The reason is that messages might get deleted without being delivered t a model.\n"
			" Otherwise, there might be dangling pointers left behind, resulting into a pretty much unfixable memory leak.\n"
			" Please also don't use a struct with pointer members.\n"
			"  They have the same problem, but there is currently no way to check for that case at compile time.");
	}

	const DataType& getData() const{
		return *reinterpret_cast<const DataType*>(m_payload.c_str());
	}

	virtual std::string getPayload() const override
	{
		std::stringstream ssr;
		const DataType& data = getData();
		ssr << data;
		return ssr.str();
	}

};

//namespace{
template<typename T>
struct isString: public std::false_type{};

template<>
struct isString<std::string>: public std::true_type{};

//}


template<typename T>
typename std::enable_if<!isString<T>::value, const T&>::type getMsgPayload(const t_msgptr& msg){
	return std::dynamic_pointer_cast<n_network::SpecializedMessage<T>>(msg)->getData();
}

template<typename T>
typename std::enable_if<isString<T>::value, std::string>::type getMsgPayload(const t_msgptr& msg){
	return msg->getPayload();
}

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
		ss << message.getSourcePort() << message.getDestinationPort() << message.getDestinationModel()
		        << message.getTimeStamp();
		std::string hashkey = ss.str();
		return std::hash<std::string>()(hashkey);
	}
};
}	// end namespace std

#endif /* SRC_NETWORK_MESSAGE_H_ */
