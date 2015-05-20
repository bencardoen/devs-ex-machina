/*
 * phold.h
 *
 *  Created on: 18 May 2015
 *      Author: matthijs
 */

#ifndef SRC_PERFORMANCE_BENCHMARKS_PHOLD_H_
#define SRC_PERFORMANCE_BENCHMARKS_PHOLD_H_

#include <stdlib.h>
#include <thread>
#include <chrono>
#include "atomicmodel.h"
#include "coupledmodel.h"

namespace n_benchmarks_phold {

struct EventPair
{
	EventPair(size_t mn, size_t pt) : m_modelNumber(mn), m_procTime(pt) {};
	size_t m_modelNumber;
	size_t m_procTime;
};

class PHOLDModelState: public n_model::State
{
public:
	PHOLDModelState();
	virtual ~PHOLDModelState();

	std::deque<EventPair> m_events;
};

class HeavyPHOLDProcessor: public n_model::AtomicModel
{
private:
	size_t m_percentageRemotes;
	size_t m_totalAtomics;
	size_t m_modelNumber;
	size_t m_iter;
	std::vector<size_t> m_local;
	std::vector<size_t> m_remote;
	int m_messageCount;
	std::vector<n_model::t_portptr> m_outs;
public:
	HeavyPHOLDProcessor(std::string name, size_t iter, size_t totalAtomics, size_t modelNumber, std::vector<size_t> local,
	        std::vector<size_t> remote, size_t percentageRemotes);
	virtual ~HeavyPHOLDProcessor();

	virtual n_model::t_timestamp timeAdvance() const;
	virtual void intTransition();
	virtual void confTransition(const std::vector<n_network::t_msgptr> & message);
	virtual void extTransition(const std::vector<n_network::t_msgptr> & message);
	virtual std::vector<n_network::t_msgptr> output() const;

	size_t getProcTime(size_t event) const;
	size_t getNextDestination(size_t event) const;
};

class PHOLD: public n_model::CoupledModel
{
public:
	PHOLD(size_t nodes, size_t atomicsPerNode, size_t iter, float percentageRemotes);
	virtual ~PHOLD();
};

} /* namespace n_benchmarks_phold */

#endif /* SRC_PERFORMANCE_BENCHMARKS_PHOLD_H_ */
