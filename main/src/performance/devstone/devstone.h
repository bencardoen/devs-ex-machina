/*
 * DEVStone.h
 *
 *  Created on: 13 Apr 2015
 *      Author: matthijs
 */

#ifndef SRC_DEVSTONE_DEVSTONE_DEVSTONE_H_
#define SRC_DEVSTONE_DEVSTONE_DEVSTONE_H_

#include <cstdlib>
#include <ctime>
#include "state.h"
#include "atomicmodel.h"
#include "coupledmodel.h"
#include "objectfactory.h"
#include "stringtools.h"

namespace n_devstone {


class ProcessorState : public n_model::State
{
public:
	size_t m_event1_counter;
	std::string m_event1;
	std::vector<std::string> m_queue;
public:
	ProcessorState();
	virtual ~ProcessorState();

	virtual std::string toString();

	friend bool operator==(const ProcessorState& lhs, const std::string rhs);
	friend bool operator==(const std::string lhs, const ProcessorState& rhs);

	std::shared_ptr<ProcessorState> copy() const;
};


class Processor : public n_model::AtomicModel
{
private:
	bool m_randomta;
public:
	Processor(std::string name, bool randomta);
	virtual ~Processor();

	virtual n_model::t_timestamp timeAdvance() const;
	virtual void intTransition();
	virtual void extTransition(const std::vector<n_network::t_msgptr> & message);
	virtual std::vector<n_network::t_msgptr> output() const;

	ProcessorState& procstate();
	const ProcessorState& procstate() const;
};

class Generator : public n_model::AtomicModel
{
public:
	Generator();
	virtual ~Generator();

	virtual n_model::t_timestamp timeAdvance() const;
	virtual void intTransition();
	virtual std::vector<n_network::t_msgptr> output() const;
};

class CoupledRecursion : public n_model::CoupledModel
{
public:
	CoupledRecursion(uint width, uint depth, uint randomta);
	virtual ~CoupledRecursion();
};


class DEVStone : public n_model::CoupledModel
{
public:
	DEVStone(uint width, uint depth, uint randomta);
	virtual ~DEVStone();
};

} /* namespace n_performance */

#endif /* SRC_DEVSTONE_DEVSTONE_DEVSTONE_H_ */
