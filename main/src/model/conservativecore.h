/*
 * conservativecore.h
 *
 *  Created on: 4 May 2015
 *      Author: Ben Cardoen -- Tim Tuijn
 */
#include <unordered_map>
#include "model/multicore.h"
#include "tools/sharedvector.h"

#ifndef SRC_MODEL_CONSERVATIVECORE_H_
#define SRC_MODEL_CONSERVATIVECORE_H_

namespace n_model {

/**
 * Stores the maximum of EOT values per kernel.
 */
typedef std::shared_ptr<n_tools::SharedVector<t_timestamp>> t_eotvector;


/**
 * A refresher on the simulation logic:
 * 	-> getMessages() //from network, local messages are handled in collectOutput().
 * 		-> queue locally for processing
 * 	-> getImminent()						// Steps 1&2 are implied here, control time <=eit
 * 	-> doOutput on imminent
 *		-> send non local output, queue the rest		// Intercepted here @sendmessage
 *	-> getPendingMail() // get messages with time < current
 *	-> transition(imminent + mail)					// +add Lookahead
 *	-> syncTime, but override so that we don't move beyond eit.	// Step 4,5, calc&set EOT/EIT, intercept time
 */

/**
 * @brief Conservative formalism implementation of parallel simulation.
 *
 * This Core/Kernel will advance time upto a safe limiting value, determined by other kernels
 * if they host models that influence a model in this kernel.
 * @source : J. J. Nutaro, Building Software for Simulation: Theory and Algorithms, with
Applications in C++. Wiley Publishing, 2010.
 *
 */
class Conservativecore: public Multicore
{
private:
	/**
	 * Earliest input time.
	 */
	t_timestamp	m_eit;

	void setEit(const t_timestamp& neweit);
        
        t_timestamp getLastMsgSentTime()const{return m_last_sent_msgtime;}

	/**
	 * Shared vector of all eots of all cores.
	 * Used in calculating max future simtime.
	 * A core only reads influencing core eots, and only writes its own.
	 */
	t_eotvector	m_distributed_eot;
        
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
	std::set<std::size_t> m_influencees;

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

public:
	Conservativecore() = delete;

	/**
	 * Forward arguments for Multicore, and link the shared vector (for eot messages).
	 * @param vc A shared vector of eot timestamps of all cores.
	 * @see Multicore
	 */
	Conservativecore(const t_networkptr& n , std::size_t coreid ,
		const n_control::t_location_tableptr& ltable, size_t cores,
		const t_eotvector& vc);
	virtual ~Conservativecore();

	/**
	 * In theory, in a distributed setting we need access to getMessages() to get an EOT value.
	 * For us, this is NOT required (shared memory).
	 * In the algoritm, we need to keep a floating max of all rcd messages per core, but this is
	 * done at sender side by sendMessage && shared vector. We have that information even before the
	 * message reaches the network.
	 */
	// void getMessages()override

	/**
	 * Executes Steps 3/4/5 of algorithms (iow update(EOT|EIT)), before forwarding to Base class syncTime
	 * to do actual advancing.
	 */
	void
	syncTime()override;

	/**
	 * Intercept any call to update the time, so that we ~never~ go higher than EIT.
	 */
	void
	setTime(const t_timestamp& newtime)override;

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
	collectOutput(std::set<std::string>& imminents)override;

	/**
	 * Allow a subclass to query a model after it has transitioned.
	 * This is required to make the Conservative Algorithm work, it needs to query the lookahead of
	 * a recently transitioned model. The alternative ( call lookahead regardless of model state) is expensive
	 * for both kernel and model.
	 */
	void
	postTransition(const t_atomicmodelptr&)override;

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

        
};

} /* namespace n_model */

#endif /* SRC_MODEL_CONSERVATIVECORE_H_ */
