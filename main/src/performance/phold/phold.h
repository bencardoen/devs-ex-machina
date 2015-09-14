/*
 * phold.h
 *
 *  Created on: 18 May 2015
 *      Author: matthijs
 */

#ifndef SRC_PERFORMANCE_PHOLD_PHOLD_H_
#define SRC_PERFORMANCE_PHOLD_PHOLD_H_

#include <stdlib.h>
#include <thread>
#include <chrono>
#include "model/atomicmodel.h"
#include "model/coupledmodel.h"

namespace n_benchmarks_phold {

// devstone uses event counters.
// The messages are "Events", which are just numbers.
#ifdef FPTIME
typedef double EventTime;
#else
typedef std::size_t EventTime;
#endif

struct EventPair
{
	EventPair(size_t mn, EventTime pt) : m_modelNumber(mn), m_procTime(pt) {};
	size_t m_modelNumber;
	EventTime m_procTime;
};

class PHOLDModelState: public n_model::State
{
public:
	PHOLDModelState();
	virtual ~PHOLDModelState();

	std::deque<EventPair> m_events;
};

typedef std::mt19937_64 t_randgen;	//don't use the default one. It's not random enough.

class HeavyPHOLDProcessor: public n_model::AtomicModel_impl
{
private:
	const size_t m_percentageRemotes;
	const size_t m_iter;
	std::vector<size_t> m_local;
	std::vector<size_t> m_remote;
	int m_messageCount;
	std::vector<n_model::t_portptr> m_outs;
	mutable t_randgen m_rand;	//This object could be a global object, but then we'd need to lock it during parallel simulation.
public:
	HeavyPHOLDProcessor(std::string name, size_t iter, size_t totalAtomics, size_t modelNumber, std::vector<size_t> local,
	        std::vector<size_t> remote, size_t percentageRemotes);
	virtual ~HeavyPHOLDProcessor();

	virtual n_network::t_timestamp timeAdvance() const;
	virtual void intTransition();
	virtual void confTransition(const std::vector<n_network::t_msgptr> & message);
	virtual void extTransition(const std::vector<n_network::t_msgptr> & message);
	virtual void output(std::vector<n_network::t_msgptr>& msgs) const;
	virtual n_network::t_timestamp lookAhead() const;

	EventTime getProcTime(size_t event) const;
	size_t getNextDestination(size_t event) const;
};

class PHOLD: public n_model::CoupledModel
{
public:
	PHOLD(size_t nodes, size_t atomicsPerNode, size_t iter, std::size_t percentageRemotes);
	virtual ~PHOLD();
};

} /* namespace n_benchmarks_phold */

#endif /* SRC_PERFORMANCE_PHOLD_PHOLD_H_ */
