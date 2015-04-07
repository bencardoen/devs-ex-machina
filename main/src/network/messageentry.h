/*
 * MessageEntry.h
 *
 *  Created on: 7 Apr 2015
 *      Author: Ben Cardoen
 */

#include "message.h"

#ifndef SRC_NETWORK_MESSAGEENTRY_H_
#define SRC_NETWORK_MESSAGEENTRY_H_

namespace n_network {

/**
 * Wrapper class for shared_ptrs to message.
 * Forwards all operator to Message
 */
class MessageEntry
{
private:
	t_msgptr	m_message;
public:
	MessageEntry() = delete;
	MessageEntry(const t_msgptr& msg):m_message(msg){;}
	virtual ~MessageEntry(){;}

	t_msgptr
	getMessage()const{return m_message;}

	friend
	bool operator<(const MessageEntry& left, const MessageEntry& right){
		return (left.getMessage()->getTimeStamp() > right.getMessage()->getTimeStamp());
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
		return (rhs < lhs);
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
		os << rhs.getMessage()->toString();
		return os;
	}
};

}

namespace std {
template<>
struct hash<n_network::MessageEntry>
{
	size_t operator()(const n_network::MessageEntry& item) const
	{
		size_t hashvalue = std::hash<n_network::Message>()(*(item.getMessage()));
		return hashvalue;
	}
};
}

#endif /* SRC_NETWORK_MESSAGEENTRY_H_ */
