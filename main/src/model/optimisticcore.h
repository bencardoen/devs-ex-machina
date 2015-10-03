/*
 * Multicore.h
 *
 *  Created on: 21 Mar 2015
 *      Author: Ben Cardoen -- Tim Tuijn
 */

#ifndef SRC_MODEL_OPTIMISTICCORE_H_
#define SRC_MODEL_OPTIMISTICCORE_H_

#include "model/core.h"
#include "model/v.h"
using n_network::MessageColor;




namespace n_model {

/**
 * Multicore implementation of Core class.
 */
class Optimisticcore: public Core
{
private:
	/**
	 * Link to network
	 */
	t_networkptr			m_network;

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
	 * Lock required for condition variable used in GVT calculation.
	 */
	std::mutex			m_cvarlock;

	/**
	 * Simulation lock
	 */
	std::mutex			m_locallock;

	/**
	 * Synchronize access to color.
	 */
	std::mutex			m_colorlock;

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
	 * Protect access to Time.
	 * @attention Needed for GVT (among other things).
	 */
	std::mutex 			m_timelock;

	/**
	 * Sent messages, stored in Front[earliest .... latest..now] Back order.
	 */
	std::deque<t_msgptr>		m_sent_messages;

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
	 * Waits until all send messages were received and we can move on with our GVT algorithm
	 * @param msg the received control message
	 */
	void
	waitUntilOK(const t_controlmsg& msg, std::atomic<bool>& rungvt);

	void
	setTred(t_timestamp);

	t_timestamp
	getTred();
        
        void
        receiveControlWorker(const t_controlmsg&, int round, std::atomic<bool>& rungvt);
        
        void
        startGVTProcess(const t_controlmsg&, int round, std::atomic<bool>& rungvt);
        
        void
        finalizeGVTRound(const t_controlmsg&, int round, std::atomic<bool>& rungvt);
        
        /**
         * In optimistic, we can only safely destroy messages after gvt has been found.
         * Clear the vector, but mark the pointers as being processed in case we ever get
         * an antimessage for it.
         * @param msgs
         * @pre msg.size()>0
         * @post msg.size() == 0
         */
        virtual void clearProcessedMessages(std::vector<t_msgptr>& msgs)override;

        
protected:
        
        /**
	 * Handle antimessage
	 * Given antimessage x ~ y (original message), if y is queued but not yet processed, annihilate and do nothing.
	 * If y is processed, trigger a revert.
	 * If y is not yet received : store x and destroy y if it arrives.
	 * @param msg the antimessage
         * @param set to true if messagetime is <= current time.
         * @attention : the case where time is equal is allready checked by msg scheduler, see implementation.
	 * @lock called by receiveMessage, which is in turn wrapped by the locked call sortIncoming()
	 */
	void
	handleAntiMessage(const t_msgptr& msg);

        /**
         * Allow msg to be traced by (among others the GVT algorithm)
         * @param msg
         */
        void
        registerReceivedMessage(const t_msgptr& msg);
        
public:
	Optimisticcore()=delete;
	/**
	 * MCore constructor
	 * @param coreid Unique sequential id (next=last+1).
	 * @param ltable Controller set loctable.
	 * @param n Link to network (message queueing system).
	 * @param cores : To properly allocate V/C vectors in Mattern, we need to know how many cores there are.
	 * @pre coreid < cores
	 * @pre loctable, network & cores are all dimensioned EXACTLY the same.
	 */
	Optimisticcore(const t_networkptr& n, std::size_t coreid, size_t cores);
	/**
	 * Deletes all sent messages.
         * @attention : The core is held by shared_ptr on Controller, so the controller's thread will always invoke the destructor.
         * Running cores while any destructor runs voids all pointer safety.
	 */
	virtual ~Optimisticcore();

	/**
	 * Pulls messages from network into mailbag (sorted by destination name
	 * @attention does not yet lock on messages access
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
	 *
	 */
	MessageColor
	getColor()override;

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
	setColor(MessageColor c)override;

	/**
	 * Sort incoming mail into time based scheduler.
	 * @locks messagelock --> do not lock func called by this function (receive, mark and friends)
	 */
	virtual void sortIncoming(const std::vector<t_msgptr>& messages);

	/**
	 * Step 1.7/1.6 in Mattern's algorithm.
         * @param round : an integer denoting the round in the algorithm starting @0, so 1==2nd round.
	 */
	virtual
	void
	receiveControl(const t_controlmsg&, int round, std::atomic<bool>& rungvt)override;

	/**
	 * Call superclass receive message, then decrements vcount (alg 1.5)
	 * @attention locked by caller on msglock
	 */
	virtual
	void receiveMessage(t_msgptr)override;

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

	/**
	 * Returns true if the network detects a pending message for any core.
	 */
	bool
	existTransientMessage()override;

	/**
	 * Set current time to new value.
	 * @synchronized
	 */
	void
	setTime(const t_timestamp&)override;

	/**
	 * Get Current simulation time.
	 * This is a timestamp equivalent to the first model scheduled to transition at the end of a simulation phase (step).
	 * @note The causal field is to be disregarded, it is not relevant here.
	 * @synchronized
	 */
	t_timestamp getTime()override;

	void
	setTerminationTime(t_timestamp)override;

	t_timestamp
	getTerminationTime()override;


//-------------statistics gathering--------------
#ifdef USE_STAT
	virtual void printStats(std::ostream& out = std::cout) const
	{
		if(getCoreID() == 0)
			m_network->printStats(out);
		Core::printStats(out);
	}
#endif
};

} /* namespace n_model */

#endif /* SRC_MODEL_OPTIMISTICCORE_H_ */
