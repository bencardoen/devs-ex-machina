/*
 * This file is part of the DEVS Ex Machina project.
 * Copyright 2014 - 2015 University of Antwerp
 * https://www.uantwerpen.be/en/
 * Licensed under the EUPL V.1.1
 * A full copy of the license is in COPYING.txt, or can be found at
 * https://joinup.ec.europa.eu/community/eupl/og_page/eupl
 *      Author: Stijn Manhaeve, Ben Cardoen, Tim Tuijn
 */

#ifndef SRC_NETWORK_MESSAGEENTRY_H_
#define SRC_NETWORK_MESSAGEENTRY_H_

#include "model/port.h"
#include "network/message.h"

namespace n_network {

/**
 * Wrapper class for ptrs to message.
 * Forwards all operator to Message
 */
class MessageEntry
{
private:
	t_msgptr	m_message;
public:
	MessageEntry() = delete;
	constexpr MessageEntry(const t_msgptr& msg):m_message(msg){;}
	~MessageEntry()=default;

        t_msgptr
	constexpr getMessage()const{return m_message;}

	friend
	bool operator<(const MessageEntry& left, const MessageEntry& right){        
		return ( *right.getMessage() < *left.getMessage() );    // Not <=, r<l for min heap conv.
	}

	friend
	bool operator==(const MessageEntry& lhs, const MessageEntry& rhs)
	{
		return (*lhs.getMessage() == *rhs.getMessage());
	}

	friend
	bool operator!=(const MessageEntry& lhs, const MessageEntry& rhs){
		return not(lhs==rhs);
	}

	friend
	bool operator>(const MessageEntry& lhs, const MessageEntry& rhs)
	{
		return (lhs.getMessage()->getTimeStamp() < rhs.getMessage()->getTimeStamp());
	}

	friend
	bool operator>=(const MessageEntry& lhs, const MessageEntry& rhs)
	{
		// a >= b implies not(a<b)
		return (not (lhs < rhs));
	}

	friend
	bool operator<=(const MessageEntry& lhs, const MessageEntry& rhs)
	{
		// a <= b  implies (not a>b)
		return (not (lhs > rhs));
	}

	friend
	std::ostream& operator<<(std::ostream& os, const MessageEntry& rhs){
		os << rhs.getMessage() << ": " << rhs.getMessage()->toString();
		return os;
	}
};

}

namespace std {
template<>
struct hash<n_network::MessageEntry>
{
	/**
	 * Use the hash function of Message (which we're pointing to).
	 */
	size_t operator()(const n_network::MessageEntry& item) const
	{
		return std::hash<n_network::Message>()(*(item.getMessage()));
	}
};
}

#endif /* SRC_NETWORK_MESSAGEENTRY_H_ */
