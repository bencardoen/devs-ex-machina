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
#include <algorithm>
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
	MessageColor			m_color;
	V				m_mcount_vector;

	/**
	 * Locks access to V vector of this core
	 */
	std::mutex			m_vlock;

	/**
	 * Simulation lock
	 */
	std::mutex			m_locallock;

	/**
	 * Lock for Tred value (as described in Mattern's GVT algorithm
	 * on pages 117 - 121 (Fujimoto) )
	 */
	std::mutex			m_tredlock;

	/**
	 * Tred is defined as the smallest time stamp of any red message sent by the processor
	 * (Mattern)
	 */
	t_timestamp			m_tred;
	std::deque<t_msgptr>		m_sent_messages;
	std::deque<t_msgptr>		m_processed_messages;

	/**
	 * Mattern 1.4, marks vcount for outgoing message
	 */
	void
	countMessage(const t_msgptr& msg);

	/**
	 * Send an antimessage.
	 * Will construct an in place copy (remeber the original is shared mem),
	 * that has the same identifying content and resend it to annihilate it's predecessor.
	 * @param msg the original message..
	 */
	void
	sendAntiMessage(const t_msgptr& msg);

	/**
	 * Handle antimessage
	 * Given antimessage x ~ y (original message), if y is queued but not yet processed, annihilate and do nothing.
	 * If y is processed, trigger a revert.
	 * If y is not yet received : store x and destroy y if it arrives.
	 * @param msg the antimessage
	 */
	void
	handleAntiMessage(const t_msgptr& msg);

	/**
	 * Waits until all send messages were received and we can move on with our GVT algorithm
	 * @param msg the received control message
	 */
	void
	waitUntilOK(const t_controlmsg& msg);

public:
	Multicore()=delete;
	Multicore(const t_networkptr&, std::size_t coreid , const n_control::t_location_tableptr& ltable, size_t cores);
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

	/**
	 * A sent message needs to be stored up until GVT.
	 */
	void markMessageStored(const t_msgptr&)override;

	/**
	 * Get the current color this core is in (Mattern's)
	 */
	MessageColor
	getColor()const{return m_color;}

	/**
	 * Revert from current time to totime.
	 * This requeues processed messages up to totime, sends antimessages for all sent
	 * messages.
	 */
	virtual
	void revert(const t_timestamp& totime)override;

	/**
	 * Set core color. (Mattern's)
	 */
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
	receiveControl(const t_controlmsg&, bool first=false)override;

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

	/**
	 * Request lock, preventing simulator from executing a simulation step.
	 */
	void
	lockSimulatorStep()override;

	/**
	 * Release simulator lock.
	 */
	void
	unlockSimulatorStep()override;

	// TODO
	void
	waitUntilAllReceived();

};

void calculateGVT(/* access to cores,*/ size_t ms, std::atomic<bool>& run);

} /* namespace n_model */

#endif /* SRC_MODEL_MULTICORE_H_ */
