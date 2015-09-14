/*
 * DEVStone.h
 *
 *  Created on: 13 Apr 2015
 *      Author: Devs Ex Machina
 *
 * This file contains the integer implementation of the devstone benchmark.
 * Note that all timestamps have been multiplied by a factor of 100 to allow for randomized time advance.
 */

#ifndef SRC_DEVSTONE_DEVSTONE_DEVSTONE_H_
#define SRC_DEVSTONE_DEVSTONE_DEVSTONE_H_

#include <cstdlib>
#include <ctime>
#include <random>
#include "model/state.h"
#include "model/atomicmodel.h"
#include "model/coupledmodel.h"
#include "tools/objectfactory.h"
#include "tools/stringtools.h"

namespace n_devstone {

// devstone uses event counters.
// The messages are "Events", which are just numbers.
typedef std::size_t Event;
#ifdef FPTIME
typedef double t_counter;
#else
typedef std::size_t t_counter;
#endif


struct ProcessorState{

	t_counter m_event1_counter;
	Event m_event1;
	std::vector<Event> m_queue;
	std::size_t m_eventsHad;

	ProcessorState();
};

}	/* namespace n_devstone */

template<>
struct ToString<n_devstone::ProcessorState>
{
	static std::string exec(const n_devstone::ProcessorState& s){
		return n_tools::toString(s.m_event1);
	}
};

namespace n_devstone {

typedef std::mt19937_64 t_randgen;

class Processor : public n_model::AtomicModel<ProcessorState>
{
private:
	static std::size_t m_numcounter;
	const bool m_randomta;
	const n_model::t_portptr m_out;
	mutable t_randgen m_rand;
public:
	const std::size_t m_num;
	Processor(std::string name, bool randomta);
	virtual ~Processor();

	virtual n_model::t_timestamp timeAdvance() const;
	virtual void intTransition();
	virtual void extTransition(const std::vector<n_network::t_msgptr> & message);
	virtual void confTransition(const std::vector<n_network::t_msgptr> & message);
	virtual void output(std::vector<n_network::t_msgptr>& msgs) const;
	virtual n_network::t_timestamp lookAhead() const;

	t_counter getProcTime(size_t event) const;
};

class Generator : public n_model::AtomicModel<void>
{
private:
	const n_model::t_portptr m_out;
public:
	Generator();
	virtual ~Generator();

	virtual n_model::t_timestamp timeAdvance() const;
	virtual void intTransition();
	virtual void output(std::vector<n_network::t_msgptr>& msgs) const;
	virtual n_network::t_timestamp lookAhead() const;
};

class CoupledRecursion : public n_model::CoupledModel
{
public:
	CoupledRecursion(std::size_t width, std::size_t depth, bool randomta);
	virtual ~CoupledRecursion();
};


class DEVStone : public n_model::CoupledModel
{
public:
	/**
	 * @param width		The width of the devstone model.
	 * @param depth		The number of layers in the devstone model
	 * @param randomta	Whether or not to use randomized time advance.
	 * 			If true, the time advance will fluctuate between 75 and 125.
	 * 			If false, the time advance will be fixed at 100.
	 */
	DEVStone(std::size_t width, std::size_t depth, bool randomta);
	virtual ~DEVStone();
};

} /* namespace n_performance */

#endif /* SRC_DEVSTONE_DEVSTONE_DEVSTONE_H_ */
