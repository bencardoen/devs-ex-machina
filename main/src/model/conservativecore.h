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
 *	-> transition(imminent + mail)					// Is ok, time <= eit
 *
 *	## Do updateEOT()
 *	## Do updateEIT()
 *
 *	-> syncTime, but override so that we don't move beyond eit.
 */

/**
 * Specialization of Multicore class. Avoids revert by simulating only up to
 * a safe point in the future.
 * @brief Conservative formalism implementation of parallel simulation.
 */
class Conservativecore: public Multicore
{
private:
	/**
	 * Earliest input time.
	 */
	t_timestamp	m_eit;

	t_timestamp getEit()const;

	/**
	 *@pre neweit > getEit().
	 */
	void setEit(const t_timestamp& neweit);

	///			|
	/// m_eot is stored in  v

	/**
	 * Shared vector of all eots of all cores.
	 * Used in calculating max future simtime.
	 */
	t_eotvector	m_distributed_eot;

	/**
	 * Record if we sent a message in the last round.
	 */
	bool		m_sent_message;
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
	 * In theory, in a distributed setting we need access to getMessages() to get a EOT value.
	 * For us, this is NOT required (shared memory).
	 * In the algoritm, we need to keep a floating max of all rcd messages per core, but this is
	 * done @sender side by sendMessage && shared vector. We have that information even before the
	 * message reaches the network.
	 */
	// void getMessages()override

	/**
	 * Step 3 of algorithm CNPDEVS
	 * Update our own EOT value with either:
	 * 	EIT, EIT+lookahead, EIT+nextscheduled
	 */
	void
	updateEOT();

	/**
	 * Steps 4/5 of algorithm CNPDEVS.
	 * Update EIT as lowest of ~maximal~ EOTS. Since all the hard work on the EOTS is allready done, this is
	 * reasonably simple.
	 */
	void
	updateEIT();

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
};

} /* namespace n_model */

#endif /* SRC_MODEL_CONSERVATIVECORE_H_ */
