/*
 * controlmessage.h
 *
 *  Created on: 10 Apr 2015
 *      Author: Ben Cardoen
 */

#ifndef SRC_NETWORK_CONTROLMESSAGE_H_
#define SRC_NETWORK_CONTROLMESSAGE_H_

#include "timestamp.h"
#include <vector>
#include <memory>
#include <algorithm>

typedef std::vector<int> t_count;

namespace n_network {
/**
 * Controlmessage, passed in 1-2 rounds during Mattern's algorithm.
 */
class ControlMessage
{
private:
	t_timestamp 		m_tmin;
	t_timestamp 		m_tred;
	t_count 		m_count;
public:
	ControlMessage(size_t cores, t_timestamp clock, t_timestamp send);
	virtual ~ControlMessage();
	t_count& getCountVector()
	{
		return m_count;
	}
	t_timestamp getTmin() const
	{
		return m_tmin;
	}
	void setTmin(t_timestamp nt)
	{
		m_tmin = nt;
	}
	t_timestamp getTred() const
	{
		return m_tred;
	}
	void setTred(t_timestamp nt)
	{
		m_tred = nt;
	}
	/**
	 * Checks if all items in the count vector are equal to 0
	 * @return true or false
	 */
	bool countIsZero()
	{
		return std::all_of(m_count.cbegin(), m_count.cend(), [](int i){ return i == 0;});
	}
};

typedef std::shared_ptr<ControlMessage> t_controlmsg;

} /* namespace n_network */

#endif /* SRC_NETWORK_CONTROLMESSAGE_H_ */
