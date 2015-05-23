/*
 * conservativecore.h
 *
 *  Created on: 4 May 2015
 *      Author: Ben Cardoen -- Tim Tuijn
 */
#include "multicore.h"
#include "sharedvector.h"

#ifndef SRC_MODEL_CONSERVATIVECORE_H_
#define SRC_MODEL_CONSERVATIVECORE_H_

namespace n_model {

/**
 * Stores the maximum of EOT values per kernel.
 */
typedef std::shared_ptr<SharedVector<t_timestamp>> t_eotvector;


/**
 * A refresher on the simulation logic:
 * 	-> getMessages() //from network.
 * 		-> queue locally for processing
 * 	IF ¬IDLE
 * 	-> getImminent()						// Steps 1&2 are implied here, control time <=eit
 * 	-> doOutput on imminent
 *		-> send non local output, queue the rest		// Intercepted here @sendmessage
 *	-> getPendingMail() // get messages with time < current
 *	-> transition(imminent + mail)					// +add Lookahead
 *
 *
 *
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

	t_timestamp getEit()const;

	void setEit(const t_timestamp& neweit);

	/**
	 * Shared vector of all eots of all cores.
	 * Used in calculating max future simtime.
	 * A core only reads influencing core eots, and only writes its own.
	 */
	t_eotvector	m_distributed_eot;

	/**
	 * Record if we sent a message in the last simulation round.
	 */
	bool		m_sent_message;

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
	 * Reset lookahead to inf, invoked after each sim run.
	 */
	void
	resetLookahead();

	/**
	 * Step 3 of algorithm CNPDEVS
	 * Update our own EOT value with either:
	 * 	EIT, EIT+lookahead, EIT+nextscheduled
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
	 * If the current EOT < than msg timestamp, update the value and remember we've sent
	 * a message.
	 * Required for CNPDEVS
	 */
	void
	sendMessage(const t_msgptr& msg)override;

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
	getEit();
};

} /* namespace n_model */

#endif /* SRC_MODEL_CONSERVATIVECORE_H_ */
