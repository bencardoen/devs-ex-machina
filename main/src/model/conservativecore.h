/*
 * conservativecore.h
 *
 *  Created on: 4 May 2015
 *      Author: Ben Cardoen -- Tim Tuijn
 */
#include <unordered_map>
#include "model/optimisticcore.h"
#include "tools/sharedvector.h"

#ifndef SRC_MODEL_CONSERVATIVECORE_H_
#define SRC_MODEL_CONSERVATIVECORE_H_

namespace n_model {

/**
 * Stores the maximum of EOT values per kernel.
 */
typedef std::shared_ptr<n_tools::SharedVector<t_timestamp>> t_eotvector;

typedef std::shared_ptr<n_tools::SharedVector<t_timestamp>> t_timevector;

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
	t_timestamp	m_eit;

	void setEit(const t_timestamp& neweit);
        
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
         * This vector is updated only after output is generated by this core.
         * Since we can't lose messages, this allows a stalled core in a deadlock to assert that, if
         * this core is influencing it, all messages that can be generated at the current time will be either 
         * pending at destination or in the network. The other core can then retrieve those messages without loss of
         * timestamps and avoid backward walking eot values.
         */
        t_timevector    m_distributed_time;
        
        /**
         * In case we're stalled, remember who has sent output and who hasn't to make
         * sure we don't send duplicate messages.
         */
        std::unordered_map<std::string, t_timestamp>    m_generated_output_at;

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
	 * Protect access to Time.
	 */
	std::mutex 			m_timelock;
        
        std::atomic<size_t>             m_zombie_rounds;
        
        /**
         * Check for each influencing core (wrt this core), if all have timestamps on null messages with values 
         * >= our time, and we ourselves have produced all output at current time.
         * @return true This core can safely transition at the current time.
         */
        bool 
        checkNullRelease();
        
        /**
         * Whenever we're stalled but not yet deadlocked, sleep or idle or ....
         * @attention: do not call in locked sections.
         */
        void
        invokeStallingBehaviour();
        
        /**
         * Queue message, revert if time <= current time or if time < current time and stalled.
         * Notifies gvt algorithm.
         * @param msg
         */
        virtual
        void receiveMessage(const t_msgptr& msg)override;

	/**
	 * Reset lookahead to inf, after at least one model has changed state we need to get a
         * new minimal lookahead.
	 */
	void
	resetLookahead();

	/**
	 * Step 3 of algorithm CNPDEVS
	 * Calculate the earliest output time of this Core (eot min of models), and publish the
         * new value. 
         * @attention : eot is a monotone ascending function (except oo).
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
        
        /**
         * If time == eit, only generate output for imminent models, but do not transition.
         * We can only get in this state if we have either a msg with timestamp == now and/or
         * a model imminent @ now. We're not allowed to transition, only to generate output (which will
         * advance our EOT, and therefore advance other's eit.
         * By the same reasoning, our eit will advance (eventually).
         * In this simulation round we do not query the network for new messages (which we're not allowed to
         * hand off to models), and we do not advance time (since time==eit, and our imminent model and or message is still
         * pending @ time=now.
         */
        void
        runSmallStepStalled();
        
        t_timestamp getLastMsgSentTime()const{return m_last_sent_msgtime;}

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

	/**
	 * Intercept any call to update the time, so that we ~never~ go higher than EIT.
         * Lock actual changing to time (since controller can request time at any moment).
	 */
	void
	setTime(const t_timestamp& newtime)override;
        
        t_timestamp
        getTime() override;

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
	 * When starting a core with serialized models from a previous run,
	 * rebuild the influencee map in addition to the superclass init code.
	 */
	void
	initExistingSimulation(const t_timestamp& loaddate)override;
        
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
	t_timestamp
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
        
        virtual std::size_t getZombieRounds()override{return m_zombie_rounds.load();}
        
        virtual void incrementZombieRounds()override{m_zombie_rounds.fetch_add(1);}
        
        virtual void resetZombieRounds()override{m_zombie_rounds.store(0);}

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
