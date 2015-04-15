/*
 * message.cpp
 *
 *  Created on: 10 Apr 2015
 *      Author: Ben Cardoen
 */
#include <iostream>
#include <cassert>
#include "message.h"


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
