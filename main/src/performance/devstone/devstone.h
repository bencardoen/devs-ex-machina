/*
 * This file is part of the DEVS Ex Machina project.
 * Copyright 2014 - 2015 University of Antwerp
 * https://www.uantwerpen.be/en/
 * Licensed under the EUPL V.1.1
 * A full copy of the license is in COPYING.txt, or can be found at
 * https://joinup.ec.europa.eu/community/eupl/og_page/eupl
 *      Author: Stijn Manhaeve, Ben Cardoen, Matthijs Van Os
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
#include "control/allocator.h"
#include "tools/frandom.h"

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


typedef n_tools::n_frandom::t_fastrng t_randgen;


class Processor : public n_model::AtomicModel<ProcessorState>
{
private:
	static std::size_t m_numcounter;
	const bool m_randomta;
	const n_model::t_portptr m_out;
	mutable t_randgen m_rand;
public:
	const std::size_t m_num;
	Processor(std::string name, bool randomta, std::size_t num);
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
public:
	const n_model::t_portptr m_out;
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
	CoupledRecursion(std::size_t width, std::size_t depth, bool randomta, std::size_t& num);
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

class DevstoneAlloc: public n_control::Allocator
{
private:
	std::size_t m_maxn;
public:
	DevstoneAlloc(): m_maxn(0){

	}
	virtual size_t allocate(const n_model::t_atomicmodelptr& ptr){
                
		auto p = std::dynamic_pointer_cast<n_devstone::Processor>(ptr);
		if(p == nullptr)
			return 0;
                
                double blocksize = (double)m_maxn/(double)coreAmount();
		
                size_t res = (1+p->m_num)/blocksize;
                LOG_DEBUG("Res = ", res, " m_num ", p->m_num, " camt ", coreAmount(), " maxn ", m_maxn);
		if(res >= coreAmount())
			res = coreAmount()-1;
		LOG_INFO("Putting model ", ptr->getName(), " in core ", res);
		return res;
	}

	virtual void allocateAll(const std::vector<n_model::t_atomicmodelptr>& models){
		m_maxn = models.size();
		assert(m_maxn && "Total amount of models can't be zero.");
		for(const n_model::t_atomicmodelptr& ptr: models)
			ptr->setCorenumber(allocate(ptr));
	}
};

} 

#endif /* SRC_DEVSTONE_DEVSTONE_DEVSTONE_H_ */
