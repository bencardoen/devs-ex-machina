/*
 * This file is part of the DEVS Ex Machina project.
 * Copyright 2014 - 2015 University of Antwerp
 * https://www.uantwerpen.be/en/
 * Licensed under the EUPL V.1.1
 * A full copy of the license is in COPYING.txt, or can be found at
 * https://joinup.ec.europa.eu/community/eupl/og_page/eupl
 *      Author: Stijn Manhaeve, Ben Cardoen, Tim Tuijn
 */

#ifndef SRC_NETWORK_MESSAGE_H_
#define SRC_NETWORK_MESSAGE_H_

#include "network/timestamp.h"
#include "tools/stringtools.h"
#include "tools/globallog.h"
#include "model/uuid.h"
#include "tools/objectfactory.h"
#include "pools/pools.h"
#include "mid.h"
#include <sstream>
#include <iosfwd>
#include <atomic>
#include <cstdint>

namespace n_network {


enum MessageColor : uint8_t{WHITE = 0, RED = 1};
// Msg status. Note that the assigned values are to be orthogonal.
// 8 bits reserved for flags
// 2^0: color
// 2^1: delete? set by optimistic core for the case
//              where a message is marked antimessage before it is received by the core
// 2^2: processed? set by optimistic core when a message has been processed (used in a transition)
// 2^3: HEAPED? The message has been put in a message scheduler in the receiving core.
// 2^4: ANTI? The message is an anti message
// 2^5: KILL? The message can be safely killed by the sending core.
// 2^6: ERASE? When found in the message scheduler, this message can be safely ignored.
enum Status : uint8_t{COLOR=MessageColor::RED, DELETE=2, PROCESSED=4, HEAPED=8, ANTI=16, KILL=32, ERASE=64};

std::ostream&
operator<<(std::ostream& os, const MessageColor& c);

/**
 * A Message representing an event passed between models.
 */
class /* __attribute__((aligned(64)))*/Message
{
protected:
	/**
	 * Time message is created (by model/port)
	 */
	t_timestamp m_timestamp;
        
        /**
         * Unique source identifier.
         */ 
        const mid             m_src_id;

        /**
         * Unique destination identifier.
         */
        const mid             m_dst_id;
        
        typedef uint8_t         t_flag_word;
    
	std::atomic<t_flag_word> m_atomic_flags;
        
        Message(const Message&) = delete;
        Message(const Message&&) = delete;
        Message& operator=(const Message&)=delete;
        Message& operator=(const Message&&)=delete;

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
	Message(const n_model::uuid& srcUUID, const n_model::uuid& dstUUID,
	const t_timestamp& time_made,
	const std::size_t& destport, const std::size_t& sourceport);

        std::size_t getDestinationPort() const
        { 
                return m_dst_id.portid();
        }
        
	std::size_t getDestinationCore() const
	{
		return m_dst_id.coreid();
	}
        
        std::size_t getDestinationModel() const
        {
                return m_dst_id.modelid();
        }
        
        std::size_t getSourceCore() const
	{
		return m_src_id.coreid();
	}
        
        std::size_t getSourceModel()const
        {
            return m_src_id.modelid();
        }
        
        std::size_t getSourcePort() const
	{ 
                return m_src_id.portid();
        }

	void setAntiMessage(bool b)
	{
        if(b)
            m_atomic_flags |= Status::ANTI;
        else
            m_atomic_flags &= ~Status::ANTI;
	}

	bool isAntiMessage() const
	{
		return (m_atomic_flags & Status::ANTI);
	}
        
	MessageColor getColor() const
	{       
                return static_cast<MessageColor>(m_atomic_flags & MessageColor::RED);
	}

	/**
	 * @brief Sets the message color
	 * @param newcolor The new message color
	 * @note The message color is part of the GVT calculation.
	 * @see getColor
	 */
	void paint(MessageColor newcolor)
	{
                if(newcolor==MessageColor::RED)
                    m_atomic_flags |= newcolor;
                else
                    m_atomic_flags &= ~MessageColor::RED;
	}

        
        void setFlag(Status newst, bool value=true)
        {
            LOG_DEBUG("setting ", newst, " flag of ", this, " ", toString(), " to ", value);
            if(value)
                m_atomic_flags |= newst;
            else
                m_atomic_flags &= ~newst;
        }

        bool flagIsSet(Status st)const
        {
            return (m_atomic_flags & st);   
            //In general should be (flag & mask) == mask, but conversion to bool serves fine.
        }

	//can't remove. Needed by tracer
	/**
	 * @brief Returns a string representation of the payload.
	 *
	 * This string representation is, among others, used by the tracers to print out
	 * trace output for the sent/received messages.
	 * @see n_network::getMsgPayload To extract the actual payload, instead of just a string representation.
	 */
	virtual std::string getPayload() const
	{
		LOG_ERROR("Message::getPayload called on base class.");
                LOG_FLUSH;
                throw std::logic_error("Message::getPayload called on base class.");
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
         * Deregister object with pool.
         * @pre This object was allocated using the registered pool. This will invoke the destructor.
         * @post The object is no longer accessible.
         */
        virtual void releaseMe()
        {
                n_tools::destroyPooledObject<typename std::remove_pointer<decltype(this)>::type>(this);
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

bool operator<(const Message& left, const Message& right);

        
bool operator<=(const Message& left, const Message& right);

        
bool operator>=(const Message& left, const Message& right);

        
bool operator>(const Message& left, const Message& right);

	
bool operator==(const Message& left, const Message& right);

	
bool operator!=(const Message& left, const Message& right);

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
class /*__attribute__((aligned(64)))*/ SpecializedMessage: public Message
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
        
        /**
         * Deregister object with pool. This will invoke the destructor.
         * @pre This object was allocated using the registered pool.
         * @post This object is no longer accessible.
         */
        virtual void releaseMe()override
        {
                n_tools::destroyPooledObject<typename std::remove_pointer<decltype(this)>::type>(this);
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


/**
 * A Wrapper object around a potentially unsafe pointer (deleted).
 * Depending on the timestamp, the object is/not safe to dereference.
 */
struct hazard_pointer{
        t_timestamp::t_time     m_msgtime;
        t_msgptr                m_ptr;
        explicit /*constexpr*/ hazard_pointer(t_msgptr msg):m_msgtime(msg->getTimeStamp().getTime()),m_ptr(msg){;}
        // Can't use cexpr because gTS() is not cexpr, and that's not possible because has a non triv ~().
};


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
			<< message.getDestinationModel()
			<< message.getSourceModel()
		    << message.getTimeStamp();
		std::string hashkey = ss.str();
		return std::hash<std::string>()(hashkey);
	}
};
}	// end namespace std

#endif /* SRC_NETWORK_MESSAGE_H_ */
