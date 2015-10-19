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
#include <atomic>

class TestCereal;

namespace n_network {

/**
 * Denote cut-type in gvt synchronization.
 */
enum MessageColor : uint8_t
{
	WHITE = 0, RED = 1
};

enum Status
{
        DELETE=2, PROCESSED=4, PENDING=8
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
	 * Port id of destination port.
	 */
	const std::size_t m_destination_port;

	/**
	 * Port id of source port.
	 */
	const std::size_t m_source_port;
	/**
	 * Is message an annihilator of the original ?
	 */
	std::atomic<bool> m_antimessage;
        
	std::atomic<uint8_t> m_color;
        
        /**
         * Edge case of original message immediately followed by antimessage, allow
         * receiver to mark this message a 'to be deleted' without ever entering any queue.
         */
        bool m_delete_flag_set;
        
        /**
         * Mark message processed.
         */
        bool m_processed;
        
        const n_model::uuid m_dst_uuid;
        
        const n_model::uuid m_src_uuid;
        
        


public:
	/**
	 * @brief Constructor for the most general message.
	 * @see Port::createMessages for a more convenient way to create messages.
	 * @param srcUUID	uuid of the model that sends the message
	 * @param dstUUID	uuid of the model that receives the message
	 * @param time_made	The timestamp at which the message is created
	 * @param destport	full name of destination port
	 * @param sourceport	full name of source port
	 * @note	Color is set by default to white.
	 */
	Message(n_model::uuid srcUUID, n_model::uuid dstUUID,
		const t_timestamp& time_made,
		const std::size_t& destport, const std::size_t& sourceport);

        std::size_t getDstPort() const
	{ 
                return m_destination_port; 
        }
        
	std::size_t getDestinationCore() const
	{
		return m_dst_uuid.m_core_id;
	}
        
        std::size_t getDestinationModel() const
        {
                return m_dst_uuid.m_local_id;
        }
        
        std::size_t getSourceCore() const
	{
		return m_src_uuid.m_core_id;
	}
        
        std::size_t getSourceModel()const
        {
                return m_src_uuid.m_local_id;
        }
        
        std::size_t getSrcPort() const
	{ 
                return m_source_port; 
        }

	// Synchronized status
	void setAntiMessage(bool b)
	{
		m_antimessage.store(b);
	}

	bool isAntiMessage() const
	{
		return m_antimessage;
	}
        
	MessageColor getColor() const
	{
#ifdef SAFETY_CHECKS
                if(m_color.load()>1)
                        throw std::logic_error("Enum to int value out of range");
#endif
                return static_cast<MessageColor>(m_color.load());       // Safe with the above check. UB otherwise.
	}

	/**
	 * @brief Sets the message color
	 * @param newcolor The new message color
	 * @note The message color is part of the GVT calculation.
	 * @see getColor
	 */
	void paint(MessageColor newcolor)
	{
		this->m_color = newcolor; // Enum to int is safe. (and type range match)
	}

        
        bool deleteFlagIsSet()const{return m_delete_flag_set;}
        
        void setDeleteFlag(){m_delete_flag_set=true;}
        
        bool isProcessed()const{return m_processed;}
        
        void setProcessed(bool b){m_processed=b;}

	//can't remove. Needed by tracer
	/**
	 * @brief Returns a string representation of the payload.
	 *
	 * This string representation is, among others, used by the tracers to print out
	 * trace output for the sent/received messages.
	 * @see n_netwrok::getMsgPayload To extract the actual payload, instead of just a string representation.
	 */
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

	/**
	 * @brief Sets the causality of the message
	 * @param causal The new causality.
	 * @note The simulator will automatically set the correct causality.
	 * @see getTimeStamp
	 * @see setTimeStamp
	 */
	void setCausality(const t_timestamp::t_causal causal)
	{
		m_timestamp = t_timestamp(m_timestamp.getTime(), causal);
	}

	virtual ~Message()
	{
                ;
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
typedef std::shared_ptr<Message> t_shared_msgptr;
typedef Message* t_msgptr;

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
	const DataType m_data;
public:
	/**
	 * @brief Constructor for a tye-specific message.
	 * @see Port::createMessages for a more convenient way to create messages.
	 * @param srcUUID	uuid of the model that sends the message
	 * @param dstUUID	uuid of the model that receives the message
	 * @param time_made The time of creation.
	 * @param destport : full name of destination port
	 * @param sourceport : full name of source port
	 * @param payload : user supplied content.
	 * @note : Color is set by default to white.
	 * @attention Core id is initialized at limits::max().
	 */
	SpecializedMessage(n_model::uuid srcUUID, n_model::uuid dstUUID, const t_timestamp& time_made, const std::size_t& destport, const std::size_t& sourceport, const DataType& data):
		Message(srcUUID, dstUUID, time_made, destport, sourceport),
		m_data(data)
	{
	}
                
        ~SpecializedMessage(){;}        

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
		std::stringstream ssr;
		const DataType& data = getData();
		ssr << data;
		return ssr.str();
	}

};


/**
 * @brief Tries to get the payload of a specific type from a message
 * @tparam T The expected type of the payload contained within the message.
 * @param msg A pointer to a message
 * @warning If the message does not contain a payload of the expected type, the simulation is allowed to abort via a segmentation fault.
 * Always make sure that the type is correct!
 */
template<typename T>
const T& getMsgPayload(const t_msgptr& msg){
        return n_tools::staticRawCast<const n_network::SpecializedMessage<T>>(msg)->getData();
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
		ss << message.getSrcPort()
			<< message.getDstPort()
			<< message.getDestinationModel()
			<< message.getSourceModel()
		        << message.getTimeStamp();
		std::string hashkey = ss.str();
		return std::hash<std::string>()(hashkey);
	}
};
}	// end namespace std

#endif /* SRC_NETWORK_MESSAGE_H_ */
