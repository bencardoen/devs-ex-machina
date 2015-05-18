/*
 * phold.h
 *
 *  Created on: 18 May 2015
 *      Author: matthijs
 */

#ifndef SRC_PERFORMANCE_BENCHMARKS_PHOLD_H_
#define SRC_PERFORMANCE_BENCHMARKS_PHOLD_H_

#include <stdlib.h>
#include "atomicmodel.h"
#include "coupledmodel.h"

namespace n_benchmarks_phold {

struct EventPair
{
	EventPair(uint mn, size_t pt) : m_modelNumber(mn), m_procTime(pt) {};
	uint m_modelNumber;
	size_t m_procTime;
};

class PHOLDModelState: public n_model::State
{
public:
	PHOLDModelState();
	virtual ~PHOLDModelState();
	std::shared_ptr<PHOLDModelState> copy();

	std::deque<EventPair> m_events;
};

class HeavyPHOLDProcessor: public n_model::AtomicModel
{
private:
	uint m_percentageRemotes;
	uint m_totalAtomics;
	uint m_modelNumber;
	uint m_iter;
	std::vector<uint> m_local;
	std::vector<uint> m_remote;
public:
	HeavyPHOLDProcessor(std::string name, uint iter, uint totalAtomics, uint modelNumber, std::vector<uint> local,
	        std::vector<uint> remote, uint percentageRemotes);
	virtual ~HeavyPHOLDProcessor();

	virtual n_model::t_timestamp timeAdvance() const;
	virtual void intTransition();
	virtual void confTransition(const std::vector<n_network::t_msgptr> & message);
	virtual void extTransition(const std::vector<n_network::t_msgptr> & message);
	virtual std::vector<n_network::t_msgptr> output() const;

	size_t getProcTime(uint event) const;
	uint getNextDestination(uint event) const;
};

class PHOLD: public n_model::CoupledModel
{
public:
	PHOLD(uint nodes, uint atomicsPerNode, uint iter, float percentageRemotes);
	virtual ~PHOLD();
};

} /* namespace n_benchmarks_phold */

#endif /* SRC_PERFORMANCE_BENCHMARKS_PHOLD_H_ */
