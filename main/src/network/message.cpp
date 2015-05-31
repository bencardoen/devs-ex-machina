/*
 * message.cpp
 *
 *  Created on: 10 Apr 2015
 *      Author: Ben Cardoen
 */
#include <iostream>
#include <cassert>
#include "message.h"
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
		m_destination_model(modeldest), m_destination_core(std::numeric_limits<std::size_t>::max()),m_source_core(std::numeric_limits<std::size_t>::max()),
		m_timestamp(time_made), m_destination_port(destport), m_source_port(sourceport), m_payload(payload), m_color(MessageColor::WHITE),m_antimessage(false)
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
	result << " from kernel " << m_source_core;
	result << " to kernel " << m_destination_core;
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
n_network::operator==(const n_network::Message& left, const n_network::Message& right){
	return(
		left.getDestinationModel()==right.getDestinationModel()
		&&
		left.getDestinationPort() == right.getDestinationPort()
		&&
		left.getSourcePort() == right.getSourcePort()
		&&
		left.getTimeStamp() == right.getTimeStamp()
	);
}

void n_network::Message::serialize(n_serialization::t_oarchive& archive)
{
	archive(m_timestamp, m_destination_model, m_destination_port, m_source_port, m_payload,
			m_destination_core, m_source_core, m_color, m_antimessage);
}

void n_network::Message::load_and_construct(n_serialization::t_iarchive& archive, cereal::construct<Message>& construct)
{
	LOG_DEBUG("Message: Load and Construct");
	std::string destination_model;
	std::size_t destination_core;
	std::size_t source_core;
	t_timestamp timestamp;
	std::string destination_port;
	std::string source_port;
	std::string payload;
	MessageColor color;
	bool  antimessage;

	archive(timestamp, destination_model, destination_port, source_port, payload,
			destination_core, source_core, color, antimessage);
	LOG_DEBUG("Message: Loaded");
	construct(destination_model, timestamp, destination_port, source_port, payload);
	LOG_DEBUG("Message: Constructed");

	construct->m_destination_core = destination_core;
	construct->m_source_core = source_core;
	construct->m_color = color;
	construct->m_antimessage = antimessage;
}
