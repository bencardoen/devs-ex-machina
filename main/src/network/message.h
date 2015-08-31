/*
 * message.h
 *
 *  Created on: 12 Mar 2015
 *      Author: Ben Cardoen, Stijn Manhaeve
 */

#ifndef SRC_NETWORK_MESSAGE_H_
#define SRC_NETWORK_MESSAGE_H_

#include "serialization/archive.h"
#include "network/timestamp.h"
#include "tools/stringtools.h"
#include "tools/globallog.h"
#include <sstream>
#include <iosfwd>

class TestCereal;

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
	friend class ::TestCereal;
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

	/**
	 * @brief Returns the destination core.
	 * @return the destination core
	 * @see setDestinationCore
	 */
	std::size_t getDestinationCore() const
	{
		return m_destination_core;
	}

	/**
	 * @brief Sets whether or not this message will act as an antimessage
	 * @param b If true, this message becomes an antimessage, otherwise, it becomes a normal message.
	 */
	void setAntiMessage(bool b)
	{
		m_antimessage = b;
	}

	/**
	 * @brief Returns whether or not this message is marked as an antimessage.
	 * @return whether or not this message is marked as an antimessage
	 * @see setAntiMessage
	 */
	bool isAntiMessage() const
	{
		return m_antimessage;
	}

	/**
	 * @brief Sets the destination core of this message.
	 * The message will be send to this core.
	 * @param dest The ID of the destination core.
	 * @note The simulator will automatically set the destination core.
	 * @note The destination core is the simulation core that simulates the destination model
	 */
	void setDestinationCore(std::size_t dest)
	{
		m_destination_core = dest;
	}

	/**
	 * @brief Returns the source core.
	 * @note The source core is the core that simulates the model that created this message.
	 */
	std::size_t getSourceCore() const
	{
		return m_source_core;
	}

	/**
	 * @brief Sets the source core of this message.
	 * @param dest The ID of the source core.
	 * @note The simulator will automatically set the source core.
	 * @note The source core is the core that simulates the model that created this message.
	 */
	void setSourceCore(std::size_t src)
	{
		m_source_core = src;
	}

	/**
	 * @brief Returns the name of the destination port.
	 * @return The full name of the destination port.
	 */
	std::string getDestinationPort() const
	{
		return n_tools::copyString(m_destination_port);
	}

	/**
	 * @brief Returns the name of the destination model.
	 * @return The name of the destination model.
	 * This model must contain the destination port. @see getDestinationPort
	 */
	std::string getDestinationModel() const
	{
		return n_tools::copyString(m_destination_model);
	}

	/**
	 * @brief Returns the name of source port of the message.
	 * @return The full name of the source port.
	 */
	std::string getSourcePort() const
	{
		return n_tools::copyString(m_source_port);
	}

	/**
	 * @brief Returns a string representation of the payload of this message.
	 * @see n_network::getMsgPayload For a more convenient way to get the payload of any message.
	 */
	virtual std::string getPayload() const
	{
		LOG_DEBUG("Message: getPayload");
		return n_tools::copyString(m_payload);
	}

	/**
	 * @brief Returns a string representation of the entire message
	 * This string representation contains information about the origin and destination of the message.
	 * It is only useful for debugging.
	 * @see getPayload For how to retrieve the string representation of the payload.
	 * @see n_network::getMsgPayload For a more convenient way to get the payload of any message.
	 */
	virtual std::string toString() const;

	/**
	 * @brief Returns the timestamp of the message. That is, the time of creation.
	 * @return the timestamp of the message.
	 */
	t_timestamp getTimeStamp() const
	{
		return m_timestamp;
	}

	/**
	 * @brief Sets the timestamp of the message
	 * @param now The new timestamp.
	 * @note The simulator will automatically set the correct timestamp.
	 * @see getTimeStamp
	 */
	void setTimeStamp(const t_timestamp& now)
	{
		m_timestamp = now;
	}

	virtual ~Message()
	{
		;
	}

	/**
	 * @brief Returns the message color.
	 * @note The message color is part of the GVT calculation.
	 */
	MessageColor getColor() const
	{
		return m_color;
	}

	/**
	 * @brief Sets the message color
	 * @param newcolor The new message color
	 * @note The message color is part of the GVT calculation.
	 * @see getColor
	 */
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
 * @tparam DataType The type of the data that will be stored as the payload of the message.
 * @note If you want to store regular c++-style strings, use the ::Message class.
 * @precondition The output operator is defined for the parameters std::ostream& and const DataType&
 * @see n_network::getMsgPayload for a way to get the data from the message without having to know the exact type of the message that was sent.
 * @see n_model::Port::createMessages for creating the correct message type.
 */
template<typename DataType>
class SpecializedMessage: public Message
{
private:
	DataType m_data;
public:
	/**
	 * @param modeldest : destination model
	 * @param time_made The time of creation.
	 * @param destport : full name of destination port
	 * @param sourceport : full name of source port
	 * @param payload : user supplied content.
	 * @note : Color is set by default to white.
	 * @attention Core id is initialized at limits::max().
	 */
	SpecializedMessage(std::string modeldest, const t_timestamp& time_made, std::string destport, std::string sourceport, const DataType& data):
		Message(modeldest, time_made, destport, sourceport, ""),
		m_data(data)
	{
	}

	/**
	 * @brief Retrieves the (non-string) payload of the message
	 * @see n_network::getMsgPayload For a more convenient way to get the payload of any message.
	 */
	const DataType& getData() const{
		return m_data;
	}

	/**
	 * @brief Returns a string representation of the payload of this message.
	 * @see n_network::getMsgPayload For a more convenient way to get the payload of any message.
	 */
	virtual std::string getPayload() const override
	{
		LOG_DEBUG("SpecializedMessage: getPayload");
		std::stringstream ssr;
		const DataType& data = getData();
		ssr << data;
		return ssr.str();
	}

};

//unnamed namespace for hiding certain implementation details
namespace{

template<typename T>
struct isString: public std::false_type{};

template<>
struct isString<std::string>: public std::true_type{};

}


/**
 * @brief Tries to get the payload of a specific type from a message
 * @tparam T The expected type of the payload contained within the message.
 * @param msg A pointer to a message
 * @warning If the message does not contain a payload of the expected type, the simulation is allowed to abort via a segmentation fault.
 * Always make sure that the type is correct!
 * This overload handles non-string types.
 */
template<typename T>
typename std::enable_if<!isString<T>::value, const T&>::type getMsgPayload(const t_msgptr& msg){
	return std::static_pointer_cast<n_network::SpecializedMessage<T>>(msg)->getData();
}

/**
 * @brief Tries to get the payload of a specific type from a message
 * @tparam T The expected type of the payload contained within the message.
 * @param msg A pointer to a message
 * @warning If the message does not contain a payload of the expected type, the simulation is allowed to abort via a segmentation fault.
 * Always make sure that the type is correct!
 * This overload handles string types.
 */
template<typename T>
typename std::enable_if<isString<T>::value, std::string>::type getMsgPayload(const t_msgptr& msg){
	return msg->getPayload();
}

} // end namespace n_network

/**
 * @brief Hash specialization for Message
 */
namespace std {
template<>
struct hash<n_network::Message>
{
	/**
	 * @brief Hash specialization for Message. Appends all const members to string, hashes result. (aka poor man's hash)
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
