/*
 * message.cpp
 *
 *  Created on: 10 Apr 2015
 *      Author: Ben Cardoen
 */
#include <iostream>
#include <string>
#include <cassert>
#include "network/message.h"


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


n_network::Message::Message(const n_model::uuid& srcUUID, const n_model::uuid& dstUUID, const t_timestamp& time_made,
				const std::size_t& destport, const std::size_t& sourceport)
		:
		m_timestamp(time_made),
                m_src_id(sourceport, srcUUID.m_core_id, srcUUID.m_local_id),
                m_dst_id(destport, dstUUID.m_core_id, dstUUID.m_local_id),
                m_atomic_flags(0u)
	{
#ifdef  SAFETY_CHECKS
                if(std::max(destport, sourceport) > n_const::port_max)
                        throw std::out_of_range("Port id out of range." + std::to_string(std::max(destport,sourceport)) );
                if(std::max(srcUUID.m_core_id, dstUUID.m_core_id) > n_const::core_max)
                        throw std::out_of_range("Core id out of range.");
                if(std::max(srcUUID.m_local_id, dstUUID.m_local_id) > n_const::model_max)
                        throw std::out_of_range("Model id out of range.");
#endif
		LOG_DEBUG("Message created with values :: ", this->toString());
	}

std::string
n_network::Message::toString() const
{
	std::stringstream result;
	result << "Message from " << this->getSourcePort() << " to " << this->getDestinationPort();
	result << " @" << m_timestamp;
	result << " from model " << getSourceModel() ;
	result << " to model " << getDestinationModel() ;
	result << " payload " << this->getPayload();
	result << " color : " << this->getColor();
	if(isAntiMessage()){
		result << " anti="<< std::boolalpha << isAntiMessage();
	}
	return result.str();
}


bool
n_network::operator!=(const n_network::Message& left, const n_network::Message& right){
	return (not (left == right));
}

bool
n_network::operator<(const n_network::Message& left, const n_network::Message& right){
    if(left == right)
        return false;
    if(left.m_timestamp < right.m_timestamp)
        return true;
    else{ 
        return false;
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
        left.m_dst_id==right.m_dst_id
        &&
        left.m_src_id==right.m_src_id
        );
 }
