/*
 * conservativecore.h
 *
 *  Created on: 4 May 2015
 *      Author: Ben Cardoen -- Tim Tuijn
 */
#include <unordered_map>
#include "model/core.h"
#include "tools/sharedvector.h"

#ifndef SRC_MODEL_CONSERVATIVECORE_H_
#define SRC_MODEL_CONSERVATIVECORE_H_

namespace n_model {

/**
 * Stores the maximum of EOT values per kernel.
 */
typedef std::shared_ptr<n_tools::SharedAtomic<t_timestamp::t_time>> t_eotvector;
typedef std::shared_ptr<n_tools::SharedAtomic<t_timestamp::t_time>> t_timevector;

/**
 * @brief Conservative formalism implementation of parallel simulation.
 *
 * This Core/Kernel will advance time upto a safe limiting value, determined by other kernels
 * if they host models that influence a model in this kernel.
 * @source : J. J. Nutaro, Building Software for Simulation: Theory and Algorithms, with
Applications in C++. Wiley Publishing, 2010.
 *
 */
class Conservativecore: public Core
{
private:
        /**
	 * Link to network
	 */
	t_networkptr			m_network;
        
	/**
	 * Earliest input time.
	 */
	t_timestamp::t_time	m_eit;

	void setEit(t_timestamp::t_time neweit);
        
        /**
         * @return true if current time == eit
         */
        bool timeStalled();

	/**
	 * Shared vector of all eots of all cores.
	 * Used in calculating max future simtime.
	 * A core only reads influencing core eots, and only writes its own.
	 */
	t_eotvector	m_distributed_eot;
        
        /**
         * Stores null message times for each core.
         * A null message time of x is a promise by a core to others it influences that any message
         * with timestamp x will be on the network. So a depending core @ time x, if stuck @EIT, need only
         * wait until all influencing have advanced nulltime >= x to proceed. This breaks deadlock in cyclic simulations where
         * lookahead does NOT span to the next event.
         */
        t_timevector    m_distributed_time;

	/**
	 * Store the cores that influence this core.
	 * This is constructed by asking each model what model it is influenced
	 * by, and for those constructing a corresponding list of
	 * core ID's.
	 * @example:
	 * 	Trafficlight 	@ core 0
	 * 	Policeman	@ core 1
	 * 	This core ==0, influencees = "Policeman", == {1}
	 */
	std::vector<std::size_t> m_influencees;

	/**
	 * Minimum lookahead for all transitioned models in a simulation step.
	 */
	t_timestamp		m_min_lookahead;
        
        /**
         * Timestamp of the last message we've sent to any other core.
         * Needed for eot calculation.
         */
        t_timestamp             m_last_sent_msgtime;
        
        /**
         * Check for each influencing core (wrt this core), if all have timestamps on null messages with values 
         * >= our time, and we ourselves have produced all output at current time.
         * @return true This core can safely transition at the current time.
         */
        bool 
        checkNullRelease();
        
        /**
         * Queue message, revert if time <= current time or if time < current time and stalled.
         * Notifies gvt algorithm.
         * @param msg
         */
        virtual
        void receiveMessage(t_msgptr msg)override;

	/**
	 * EOT = min(imminent, messagetime, sent+eps, lookahead).
         * If Lookahead stalls the next time time, allow EOT == nulltime + eps iff all influencing have reached that time.
         * @attention : eot is a monotone ascending function (except oo).
         * @throws : logic_error if eot moves backward.
	 */
	void
	updateEOT();

	/**
	 * Steps 4/5 of algorithm CNPDEVS.
	 * Update EIT as lowest of ~maximal~ EOTS. Since all the hard work on the EOTS is already done, this is
	 * reasonably simple. Furthermore, we only look at EOTS' of influencing kernels.
	 */
	void
	updateEIT();
        
        void
        runSmallStepStalled();
        
        /**
         * Required for eot calculation. Remember timestamp of last sent message.
         */
        t_timestamp getLastMsgSentTime()const{return m_last_sent_msgtime;}
        
        /**
         * Get last null message time. Since this is a read of our own write, we
         * can use relaxed ordering here.
         */
        t_timestamp::t_time getNullTime()const{return m_distributed_time->get(this->getCoreID(), std::memory_order_relaxed);}
        
        void setNullTime(t_timestamp::t_time nlt){m_distributed_time->set(this->getCoreID(), nlt);}
        
         /**
         * @attention : synchronized (write/read)
         */
        void setEot(t_timestamp ntime);
        
         /**
         * @see getNullTime()
         */
        t_timestamp getEot()const{return m_distributed_eot->get(this->getCoreID(), std::memory_order_relaxed);} // read/read

public:
	Conservativecore() = delete;

	/**
	 * Forward arguments for Multicore, and link the shared vector (for eot messages).
	 * @param vc A shared vector of eot timestamps of all cores.
	 * @see Multicore
	 */
	Conservativecore(const t_networkptr& n, std::size_t coreid, std::size_t totalCores,
		const t_eotvector& vc, const t_timevector& tc);
	virtual ~Conservativecore();

	/**
	 * In theory, in a distributed setting we need access to getMessages() to get an EOT value.
	 * For us, this is NOT required (shared memory).
	 * In the algorithm, we need to keep a floating max of all rcd messages per core, but this is
	 * done at sender side by sendMessage && shared vector. We have that information even before the
	 * message reaches the network.
	 */
	void getMessages()override;

	/**
	 * Executes Steps 3/4/5 of algorithms (iow update(EOT|EIT)), before forwarding to Base class syncTime
	 * to do actual advancing.
	 */
	void
	syncTime()override;
        
        void sortIncoming(const std::vector<t_msgptr>& messages);
        
        virtual
        void setTime(const t_timestamp&)override;

	/**
	 * @brief Condense model depency graph to integer-index vector.
	 *
	 * We ask each of our own models for the names of the models they are influenced by
	 * Next, for each of that list, we query the lookuptable to see where they are
	 * allocated to get a mapping from Name <> Kernel location. This means that we know at
	 * once who we're influenced by, so we only need to look at those kernel's EOTS.
	 */
	void
	buildInfluenceeMap();

	/**
	 * Link creation of influencee-map into call to init.
	 * @attention : call once and once only.
	 */
	void
	init()override;
        
        /**
	 * Collect output from imminent models, sort them in the mailbag by destination name.
         * Marks those models that have generated output, since we may visit them several times.
	 * @attention : generated messages (events) are timestamped by the current core time.
	 */
	virtual void
	collectOutput(std::vector<t_raw_atomic>& imminents)override;

	/**
	 * Return current Earliest input time.
	 */
	t_timestamp::t_time
	getEit()const;
        
        /**
         * If we're at eit==time, only generate output (once) for imminent models.
         * Else, perform a standard simulation step.
         */
        virtual
	void
	runSmallStep()override;
        
        /**
         * Records timestamp for eot, leaves actual handling to superclass.
         */
        virtual void sendMessage(const t_msgptr&)override;
        
        /**
	 * Returns true if the network detects a pending message for any core.
	 */
	bool
	existTransientMessage()override;
        
        /**
         * If time >= min_lookahead, calculate the next minimal value.
         * @attention We need to query all models (regardless if they have made a transition this turn), to avoid skipping
         * minimal values. See the inline docs for a counterexample.
         * LA is needed by eot calculation, and best done before time advances.
         * @pre lookahead() returns a non zero value (@see t_timestamp::isZero())
         * @post the lookahead value of this core is updated to a new floating minimum (in absolute time)
         * @throw std::logic_error if any model returns a zero lookahead value.
         */
        void
        calculateMinLookahead();

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

#endif /* SRC_MODEL_CONSERVATIVECORE_H_ */
