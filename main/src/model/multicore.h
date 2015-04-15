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
#include "message.h"
#include <deque>
#include "v.h"
using n_network::MessageColor;


namespace n_model {

/**
 * Multicore implementation of Core class.
 */
class Multicore: public Core
{
private:
	t_networkptr			m_network;
	n_control::t_location_tableptr	m_loctable;
	MessageColor		m_color;
	t_V				m_mcount_vector;
	/**
	 * Locks access to shared vector that counts recvd, sent msgs.
	 */
	std::mutex&			m_vlock;
	/**
	 * Simulation lock
	 */
	std::mutex			m_locallock;
	t_timestamp			m_tmin;
	std::deque<t_msgptr>		m_sent_messages;
	std::deque<t_msgptr>		m_processed_messages;

	/**
	 * Mattern 1.4, marks vcount for outgoing message
	 */
	void
	countMessage(const t_msgptr& msg);

	/**
	 * Send an antimessage based on this one.
	 */
	void
	sendAntiMessage(const t_msgptr& msg);

	void
	handleAntiMessage(const t_msgptr& msg);

public:
	Multicore()=delete;
	Multicore(const t_networkptr&, std::size_t coreid , const n_control::t_location_tableptr& ltable, std::mutex& vlock, size_t cores);
	/**
	 * Resets ptrs to network and locationtable.
	 */
	virtual ~Multicore();

	/**
	 * Pulls messages from network into mailbag (sorted by destination name
	 */
	void getMessages()override;

	/**
	 * Lookup message destination core, fix address field and send to network.
	 */
	void sendMessage(const t_msgptr&)override;

	void markMessageStored(const t_msgptr&)override;

	MessageColor
	getColor()const{return m_color;}

	virtual
	void revert(const t_timestamp& totime)override;

	void
	setColor(MessageColor c){m_color = c;}

	/**
	 * Sort network received messages into local queues.
	 */
	virtual void sortIncoming(const std::vector<t_msgptr>& messages);

	/**
	 * Step 1.7/1.6 in Mattern's algorithm.
	 */
	virtual
	void
	receiveControl(const t_controlmsg&)override;

	/**
	 * Call superclass receive message, then decrements vcount (alg 1.5)
	 * @attention locked
	 */
	virtual
	void receiveMessage(const t_msgptr&)override;

	/**
	 * If a model received a set of messages, store these message as processed in the core.
	 */
	void markProcessed(const std::vector<t_msgptr>&) override;

	/**
	 * Sets new gvt.
	 * This clears all processed messages time < newgvt, all send messages < newgvt
	 * @pre newgvt >= this->getGVT()
	 */
	virtual
	void setGVT(const t_timestamp&)override;

	void
	lockSimulatorStep()override;

	void
	unlockSimulatorStep()override;

	// TODO
	void
	waitUntilAllReceived();

};

void calculateGVT(/* access to cores,*/ size_t ms, std::atomic<bool>& run);

} /* namespace n_model */

#endif /* SRC_MODEL_MULTICORE_H_ */
