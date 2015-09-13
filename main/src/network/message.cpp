/*
 * message.cpp
 *
 *  Created on: 10 Apr 2015
 *      Author: Ben Cardoen
 */
#include <iostream>
#include <cassert>
#include "network/message.h"
#include "cereal/types/string.hpp"
#include "cereal/types/map.hpp"
#include "cereal/types/vector.hpp"
#include "cereal/types/memory.hpp"


std::ostream&
n_network::operator<<(std::ostream& os, const n_network::MessageColor& c){
	switch(c){
	case MessageColor::WHITE:
		return os << "WHITE";
	case MessageColor::RED:
		return os << "RED";
	default:
		assert(false && "unsupported messagecolor");
		return os;// silence compiler.
	}
}


n_network::Message::Message(std::string modeldest, const t_timestamp& time_made, std::string destport, std::string sourceport,const std::string& payload)
		:
		m_destination_model(modeldest),
		m_timestamp(time_made), m_destination_port(destport), m_source_port(sourceport), m_payload(payload), m_color(MessageColor::WHITE),m_antimessage(false),
                m_dst_uuid(std::numeric_limits<std::size_t>::max(),std::numeric_limits<std::size_t>::max()),
                m_src_uuid(std::numeric_limits<std::size_t>::max(),std::numeric_limits<std::size_t>::max())
	{
		LOG_DEBUG("initializing message with modeldest ", modeldest);
	}


std::string
n_network::Message::toString() const
{
	std::stringstream result;
	result << "Message from " << this->getSourcePort() << " to " << this->getDestinationPort();
	result << " @" << m_timestamp;
	result << " to model " << this->getDestinationModel() ;
	result << " from kernel " << getSourceCore();
	result << " to kernel " << getDestinationCore();
	result << " payload " << this->getPayload();
	result << " color : " << this->getColor();
	if(m_antimessage){
		result << " anti="<< std::boolalpha << m_antimessage;
	}
	return result.str();
}


bool
n_network::operator!=(const n_network::Message& left, const n_network::Message& right){
		return (not (left == right));
	}

bool
n_network::operator<(const n_network::Message& left, const n_network::Message& right){
        if(left.m_timestamp < right.m_timestamp)
                return true;
        else{ // >=
                if(left.m_timestamp==right.m_timestamp) // ==
                        return left.m_destination_model < right.m_destination_model;    // TODO check if we need even more...
                else    
                        return false;   // >
        }
               
}

bool
n_network::operator<=(const n_network::Message& left, const n_network::Message& right){
	return (left < right || left==right);
}


bool
n_network::operator>=(const n_network::Message& left, const n_network::Message& right){
	return (left > right || left==right);
}


bool
n_network::operator>(const n_network::Message& left, const n_network::Message& right){
	return !( left<= right);
}

 bool
n_network::operator==(const n_network::Message& left, const n_network::Message& right){
        // short circuit, tstamp will fail first.
        return(
                left.getTimeStamp() == right.getTimeStamp()
                &&
                left.getDestinationModel()==right.getDestinationModel()
                &&
                left.getDestinationPort() == right.getDestinationPort()
                &&
                left.getSourcePort() == right.getSourcePort()
                );
 }

void n_network::Message::serialize(n_serialization::t_oarchive& archive)
{
	archive(m_timestamp, m_destination_model, m_destination_port, m_source_port, m_payload,
			 m_color, m_antimessage);
}

void n_network::Message::load_and_construct(n_serialization::t_iarchive& archive, cereal::construct<Message>& construct)
{
        LOG_ERROR("NO LONGER FUNCTIONAL");
	LOG_DEBUG("Message: Load and Construct");
	std::string destination_model;
	//std::size_t destination_core;
	//std::size_t source_core;
	t_timestamp timestamp;
	std::string destination_port;
	std::string source_port;
	std::string payload;
	MessageColor color;
	bool  antimessage;

	archive(timestamp, destination_model, destination_port, source_port, payload,
			 color, antimessage);
	LOG_DEBUG("Message: Loaded");
	construct(destination_model, timestamp, destination_port, source_port, payload);
	LOG_DEBUG("Message: Constructed");

	//construct->m_destination_core = destination_core;
	//construct->m_source_core = source_core;
	construct->m_color = color;
	construct->m_antimessage = antimessage;
}
