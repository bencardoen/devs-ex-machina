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
#include "model/uuid.h"
#include "tools/objectfactory.h"
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
	 * Color in synchronization algorithms.
	 * @default WHITE
	 * @see MessageColor
	 */
	MessageColor m_color;

	/**
	 * Is message an annihilator of the original ?
	 */
	bool m_antimessage;
        
        n_model::uuid m_dst_uuid;
        
        n_model::uuid m_src_uuid;


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
	Message(n_model::uuid srcUUID, n_model::uuid dstUUID, const t_timestamp& time_made, const std::string& destport, const std::string& sourceport);

        n_model::uuid&
        getSrcUUID(){return m_src_uuid;}

//        constexpr
        const n_model::uuid&
        getSrcUUID()const{return m_src_uuid;}
        
        n_model::uuid&
        getDstUUID(){return m_dst_uuid;}

//        constexpr
        const n_model::uuid&
        getDstUUID()const{return m_dst_uuid;}
        
	/**
	 * @brief Returns the destination core.
	 * @return the destination core
	 * @see setDestinationCore
	 */
	std::size_t getDestinationCore() const
	{
		return m_dst_uuid.m_core_id;
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
	 * @brief Returns the source core.
	 * @note The source core is the core that simulates the model that created this message.
	 */
	std::size_t getSourceCore() const
	{
		return m_src_uuid.m_core_id;
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
	 * @brief Returns the name of source port of the message.
	 * @return The full name of the source port.
	 */
	std::string getSourcePort() const
	{
		return n_tools::copyString(m_source_port);
	}

	//TODO remove
	virtual std::string getPayload() const
	{
		LOG_ERROR("Message::getPayload called on base class.");
		return "";
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
        bool operator<(const Message& left, const Message& right);
        
        friend
        bool operator<=(const Message& left, const Message& right);
        
        friend
        bool operator>=(const Message& left, const Message& right);
        
        friend
        bool operator>(const Message& left, const Message& right);

	friend
	bool operator==(const Message& left, const Message& right);

	friend
	bool operator!=(const Message& left, const Message& right);
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
	SpecializedMessage(n_model::uuid srcUUID, n_model::uuid dstUUID, const t_timestamp& time_made, const std::string& destport, const std::string& sourceport, const DataType& data):
		Message(srcUUID, dstUUID, time_made, destport, sourceport),
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
const T& getMsgPayload(const t_msgptr& msg){
	return n_tools::staticCast<const n_network::SpecializedMessage<T>>(msg)->getData();
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
		ss << message.getSourcePort()
			<< message.getDestinationPort()
			<< message.getDstUUID().m_local_id
			<< message.getSrcUUID().m_local_id
		        << message.getTimeStamp();
		std::string hashkey = ss.str();
		return std::hash<std::string>()(hashkey);
	}
};
}	// end namespace std

#endif /* SRC_NETWORK_MESSAGE_H_ */
