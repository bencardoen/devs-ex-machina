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
	/**
	 * Link to network
	 */
	t_networkptr			m_network;

	/**
	 *
	 */
	n_control::t_location_tableptr	m_loctable;

	/**
	 * This core's current color state.
	 */
	MessageColor			m_color;

	/**
	 * Local count vector for Mattern.
	 */
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
	 * Message lock. Excludes access to [sent|processed|pending]
	 */
	std::mutex			m_msglock;

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

	/**
	 * Sent messages, stored in Front[earliest .... latest..now] Back order.
	 */
	std::deque<t_msgptr>		m_sent_messages;

	/**
	 * Processed messages, stored in Front[earliest....latest..now] Back order.
	 */
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
	 * @param msg the original message.
	 * @attention : triggers 1.4 Mattern
	 */
	void
	sendAntiMessage(const t_msgptr& msg);

	/**
	 * Handle antimessage
	 * Given antimessage x ~ y (original message), if y is queued but not yet processed, annihilate and do nothing.
	 * If y is processed, trigger a revert.
	 * If y is not yet received : store x and destroy y if it arrives.
	 * @param msg the antimessage
	 * @lock called by receiveMessage, which is in turn wrapped by the locked call sortIncoming()
	 */
	void
	handleAntiMessage(const t_msgptr& msg);

	/**
	 * Waits until all send messages were received and we can move on with our GVT algorithm
	 * @param msg the received control message
	 */
	void
	waitUntilOK(const t_controlmsg& msg);


	/**
	 * Set the color of msg with this core's color.
	 */
	virtual
	void
	paintMessage(const t_msgptr& msg)override;

public:
	Multicore()=delete;
	/**
	 * MCore constructor
	 * @param coreid Unique sequential id (next=last+1).
	 * @param ltable Controller set loctable.
	 * @param n Link to network (message queueing system).
	 * @param cores : To properly allocate V/C vectors in Mattern, we need to know how many cores there are.
	 * @pre coreid < cores
	 * @pre loctable, network & cores are all dimensioned EXACTLY the same.
	 */
	Multicore(const t_networkptr& n , std::size_t coreid , const n_control::t_location_tableptr& ltable, size_t cores);
	/**
	 * Resets ptrs to network and locationtable.
	 */
	virtual ~Multicore();

	/**
	 * Pulls messages from network into mailbag (sorted by destination name
	 * @attention does not yet lock on messages acces
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
	 * @pre totime >= this->getGVT();
	 * @lock called during simlock(smallStep) && msglock (sort->receive)
	 */
	virtual
	void revert(const t_timestamp& totime)override;

	/**
	 * Set core color. (Mattern's)
	 */
	void
	setColor(MessageColor c){m_color = c;}

	/**
	 * Sort incoming mail into time based scheduler.
	 * @locks messagelock --> do not lock func called by this function (receive, mark and friends)
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
	 * @attention locked by caller on msglock
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
	 * @locks : acquires simulatorlock
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

	/**
	 * Request lock on [pending|sent|processed] messages.
	 */
	virtual
	void
	lockMessages()override;

	/**
	 * Release lock on [pending|sent|processed] messages.
	 */
	virtual
	void
	unlockMessages()override;
};

} /* namespace n_model */

#endif /* SRC_MODEL_MULTICORE_H_ */
