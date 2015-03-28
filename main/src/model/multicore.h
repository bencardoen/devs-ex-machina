/*
 * Multicore.h
 *
 *  Created on: 21 Mar 2015
 *      Author: Ben Cardoen
 */

#ifndef SRC_MODEL_MULTICORE_H_
#define SRC_MODEL_MULTICORE_H_

#include "core.h"
#include "locationtable.h"


namespace n_model {

/**
 * Multicore implementation of Core class.
 */
class Multicore: public Core
{
private:
	std::mutex		m_lock;
	t_networkptr		m_network;
	n_control::t_location_tableptr	m_loctable;
public:
	Multicore()=delete;
	Multicore(const t_networkptr&, std::size_t coreid , const n_control::t_location_tableptr& ltable);
	virtual ~Multicore(){;}

	/**
	 * Overridden , pulls messages from network into mailbag (sorted by destination name
	 */
	void getMessages(std::unordered_map<std::string, std::vector<t_msgptr>>& mailbag)override;

	/**
	 * Lookup message destination core, fix address field and send to network.
	 */
	void sendMessage(const t_msgptr&)override;

	/**
	 * Sort pulled messages into mailbag.
	 */
	virtual void sortIncoming(std::unordered_map<std::string, std::vector<t_msgptr>>&, const std::vector<t_msgptr>& messages);

	virtual
	bool isMessageLocal(const t_msgptr&)const;
};

} /* namespace n_model */

#endif /* SRC_MODEL_MULTICORE_H_ */
