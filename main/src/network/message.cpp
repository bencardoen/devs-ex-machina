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


n_network::Message::Message(n_model::uuid srcUUID, n_model::uuid dstUUID, const t_timestamp& time_made,
				const std::size_t& destport, const std::size_t& sourceport)
		:
		m_timestamp(time_made),
		m_destination_port(destport),
		m_source_port(sourceport),
		m_color(MessageColor::WHITE),
		m_antimessage(false),
                m_dst_uuid(dstUUID),
                m_src_uuid(srcUUID)
	{
		LOG_DEBUG("initializing message with modeldest ", dstUUID);
	}


std::string
n_network::Message::toString() const
{
	std::stringstream result;
	result << "Message from " << this->getSrcPort() << " to " << this->getDstPort();
	result << " @" << m_timestamp;
	result << " from model " << getSrcUUID() ;
	result << " to model " << getDstUUID() ;
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
                        return left.m_dst_uuid.m_local_id < right.m_dst_uuid.m_local_id;    // TODO check if we need even more...
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
                left.m_dst_uuid==right.m_dst_uuid
                &&
                left.m_src_uuid==right.m_src_uuid
                &&
                left.getDstPort() == right.getDstPort()
                &&
                left.getSrcPort() == right.getSrcPort()
                );
 }
