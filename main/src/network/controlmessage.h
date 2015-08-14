/*
 * controlmessage.h
 *
 *  Created on: 10 Apr 2015
 *      Author: Ben Cardoen - Tim Tuijn
 */

#ifndef SRC_NETWORK_CONTROLMESSAGE_H_
#define SRC_NETWORK_CONTROLMESSAGE_H_

#include "network/timestamp.h"
#include <vector>
#include <memory>
#include <algorithm>
#include "tools/globallog.h"

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
	bool			m_gvt_found;
	t_timestamp		m_gvt;
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

	const t_timestamp& getGvt() const
	{
		return m_gvt;
	}

	void setGvt(const t_timestamp& gvt)
	{
		m_gvt = gvt;
	}

	bool isGvtFound() const
	{
		return m_gvt_found;
	}

	void setGvtFound(bool gvtFound)
	{
		m_gvt_found = gvtFound;
	}
        
        /**
         * Write the full state of this controlmessage to LOG.
         * Required if we're to have any hope of debugging GVT>
         */
        void logMessageState();
};

typedef std::shared_ptr<ControlMessage> t_controlmsg;

} /* namespace n_network */

#endif /* SRC_NETWORK_CONTROLMESSAGE_H_ */
