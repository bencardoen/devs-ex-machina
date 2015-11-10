/*
 * optimisticcore.h.h
 *
 *  Created on: 21 Mar 2015
 *      Author: Ben Cardoen -- Tim Tuijn -- Stijn Manhaeve
 */

#ifndef SRC_MODEL_OPTIMISTICCORE_H_
#define SRC_MODEL_OPTIMISTICCORE_H_

#include "model/core.h"
#include "model/v.h"
using n_network::MessageColor;




namespace n_model {

/**
 * Optimistic synchronizing simulation core.
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
	 * Simulation lock
	 */
	std::mutex			m_locallock;

	/**
	 * Synchronize access to color.
	 */
	std::mutex			m_colorlock;

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
         * Tmin is <= first unprocessed message, which is equivalent of time().
         * @synchronized
         */
        std::atomic<t_timestamp::t_time>        m_tmin;

	/**
	 * Sent messages, stored in Front[earliest .... latest..now] Back order.
	 */
        std::deque<t_msgptr>                    m_sent_messages;
        /**
         * Sent anti messages, must be kept separate from the regular messages
         */
        std::deque<t_msgptr>                    m_sent_antimessages;

	bool m_removeGVTMessages;
        
        std::deque<t_msgptr>                    m_processed_messages;

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
        
        t_timestamp
        getTMin()const{return m_tmin.load();}
        
        void
        setTMin(const t_timestamp& t){m_tmin.store(t.getTime());}
        
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
        
        /**
         * Garbage collect @ chosen time.
         * @pre is called by thread that simulates. 
         */
        void gcCollect();

        
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

        void runSmallStep() override;

        void shutDown() override;

	/**
	 * Pulls messages from network into mailbag (sorted by destination name
	 * @attention does not yet lock on messages access
	 */
	void getMessages()override;

	/**
	 * Sort all mail.
	 */
	virtual void
	sortMail(const std::vector<t_msgptr>& messages, std::size_t& msgCount) override;

	/**
	 * Lookup message destination core, fix address field and send to network.
	 */
	void sendMessage(t_msgptr)override;

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
	 * Returns true if the network detects a pending message for any core.
	 */
	bool
	existTransientMessage()override;

	/**
	 * Set current time to new value.
	 * @synchronized on tmin, updates it to new value.
	 */
	void
	setTime(const t_timestamp&)override;

        

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
