/*
 * This file is part of the DEVS Ex Machina project.
 * Copyright 2014 - 2015 University of Antwerp
 * https://www.uantwerpen.be/en/
 * Licensed under the EUPL V.1.1
 * A full copy of the license is in COPYING.txt, or can be found at
 * https://joinup.ec.europa.eu/community/eupl/og_page/eupl
 *      Author: Stijn Manhaeve, Ben Cardoen
 */

#ifndef SRC_PERFORMANCE_HIGHINTERCONNECT_HINTERCONNECT_H_
#define SRC_PERFORMANCE_HIGHINTERCONNECT_HINTERCONNECT_H_

#include "model/atomicmodel.h"
#include "model/coupledmodel.h"
#include "tools/objectfactory.h"
#include "control/allocator.h"
#include <random>
#include "tools/frandom.h"

namespace n_interconnect {

#ifdef FPTIME
typedef double t_counter;
#else
typedef std::size_t t_counter;
#endif
typedef std::mt19937_64 t_randgen;
typedef boost::random::taus88 t_seedrandgen;    //this random generator will be used to generate the initial seeds
//it MUST be diferent from the regular t_randgen
static_assert(!std::is_same<t_randgen, t_seedrandgen>::value, "The rng for the seed can't be the same random number generator as the one for he random events.");


class GeneratorState
{
public:
	t_counter m_count;
    mutable t_randgen m_rand;
public:
	GeneratorState(t_counter count = 0u);
};

}	/* namespace n_interconnect */

template<>
struct ToString<n_interconnect::GeneratorState>
{
	static std::string exec(const n_interconnect::GeneratorState& s){
		return n_tools::toString(s.m_count);
	}
};

namespace n_interconnect {

class Generator: public n_model::AtomicModel<GeneratorState>
{
private:
	const bool m_randomta;
public:
	n_model::t_portptr m_out;
	n_model::t_portptr m_in;

	Generator(const std::string& name, std::size_t seed, bool randTa);
	void adjustCounter();	//changes current state

	virtual n_model::t_timestamp timeAdvance() const;
	virtual void intTransition();
	virtual void extTransition(const std::vector<n_network::t_msgptr> & message);
	virtual void confTransition(const std::vector<n_network::t_msgptr> & message);
	virtual void output(std::vector<n_network::t_msgptr>& msgs) const;
	virtual n_network::t_timestamp lookAhead() const;

	virtual ~Generator();
};

class HighInterconnect : public n_model::CoupledModel
{
public:
	/**
	 * @param width		The width of the high interconnect model.
	 * @param randomta	Whether or not to use randomized time advance.
	 * 			If true, the time advance will fluctuate between 1 and 100.
	 * 			If false, the time advance will be fixed at 100.
	 * @param startSeed the start seed in the case of random time advance
	 */
	HighInterconnect(std::size_t width, bool randomta, std::size_t startSeed)
	: CoupledModel("High Interconnect")
	{
	    t_seedrandgen getSeed;
	    getSeed.seed(startSeed);
		std::vector<n_model::t_atomicmodelptr> ptrs;
		for(std::size_t i = 0; i < width; ++i){
			n_model::t_atomicmodelptr pt = n_tools::createObject<Generator>(std::string("Generator") + n_tools::toString(i), getSeed(), randomta);
                        ptrs.push_back(pt);
                        addSubModel(pt);
		}
		for(std::size_t i = 0; i < width; ++i){
			for(std::size_t j = 0; j < width; ++j){
				if(i != j){
					connectPorts(n_tools::staticCast<Generator>(ptrs[i])->m_out,
						n_tools::staticCast<Generator>(ptrs[j])->m_in);
                                }
			}
		}
	}
	virtual ~HighInterconnect(){};
};

class InterconnectAlloc: public n_control::Allocator
{
private:
	std::size_t m_maxn;
public:
	InterconnectAlloc(): m_maxn(0){

	}
	virtual size_t allocate(const n_model::t_atomicmodelptr&){
		return 0;
	}

	virtual void allocateAll(const std::vector<n_model::t_atomicmodelptr>& models){
		m_maxn = models.size();
		assert(m_maxn && "Total amount of models can't be zero.");
		std::size_t i = 0;
		for(const n_model::t_atomicmodelptr& ptr: models)
			ptr->setCorenumber((i++)%coreAmount());
	}
};


} /* namespace n_interconnect */

#endif /* SRC_PERFORMANCE_HIGHINTERCONNECT_HINTERCONNECT_H_ */
