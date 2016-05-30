/*
 * This file is part of the DEVS Ex Machina project.
 * Copyright 2014 - 2015 University of Antwerp
 * https://www.uantwerpen.be/en/
 * Licensed under the EUPL V.1.1
 * A full copy of the license is in COPYING.txt, or can be found at
 * https://joinup.ec.europa.eu/community/eupl/og_page/eupl
 *      Author: Stijn Manhaeve, Ben Cardoen, Tim Tuijn
 */

#include <iostream>
#include <string>
#include <cassert>
#include <bitset>
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
                        throw std::out_of_range("Port id out of range." );
                if(std::max(srcUUID.m_core_id, dstUUID.m_core_id) > n_const::core_max)
                        throw std::out_of_range("Core id out of range.");
                if(std::max(srcUUID.m_local_id, dstUUID.m_local_id) > n_const::model_max)
                        throw std::out_of_range("Model id out of range.");
#endif
//		LOG_DEBUG("Message created with values :: ", this->toString());
	}

std::string
n_network::Message::toString() const
{
	std::stringstream result;
	result << "Message from port " << this->getSourcePort() << " to " << this->getDestinationPort();
	result << " @" << m_timestamp;
	result << " from model " << getSourceModel() ;
	result << " to model " << getDestinationModel() ;
        result << " from core " << getSourceCore() ;
        result << " to core " << getDestinationCore() ;
	result << " payload " << this->getPayload();
	result << " color : " << this->getColor();
	result << " flags: " << std::bitset<8>(m_atomic_flags.load());
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
        return left.m_timestamp < right.m_timestamp;
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
        return  (
                left.getTimeStamp() == right.getTimeStamp()
                &&
                left.m_dst_id==right.m_dst_id
                &&
                left.m_src_id==right.m_src_id
                );
 }
