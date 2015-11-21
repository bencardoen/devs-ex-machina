/*
 * hinterconnect.h
 *
 *  Created on: Sep 5, 2015
 *      Author: lttlemoi
 */

#ifndef SRC_PERFORMANCE_HIGHINTERCONNECT_HINTERCONNECT_H_
#define SRC_PERFORMANCE_HIGHINTERCONNECT_HINTERCONNECT_H_

#include "model/atomicmodel.h"
#include "model/coupledmodel.h"
#include "tools/objectfactory.h"
#include "control/allocator.h"
#include <random>

namespace n_interconnect {

#ifdef FPTIME
typedef double t_counter;
#else
typedef std::size_t t_counter;
#endif
typedef std::mt19937_64 t_randgen;

class GeneratorState
{
public:
	t_counter m_count;
	std::size_t m_seed;
public:
	GeneratorState(t_counter count = 0u, std::size_t seed = 0u);
};

}	/* namespace n_interconnect */

template<>
struct ToString<n_interconnect::GeneratorState>
{
	static std::string exec(const n_interconnect::GeneratorState& s){
		return n_tools::toString(s.m_count) + ", " + n_tools::toString(s.m_seed);
	}
};

namespace n_interconnect {

class Generator: public n_model::AtomicModel<GeneratorState>
{
private:
	const bool m_randomta;
	mutable t_randgen m_rand;
public:
	n_model::t_portptr m_out;
	n_model::t_portptr m_in;

	Generator(const std::string& name, std::size_t seed, bool randTa);
	void adjustCounter(std::size_t seed);	//changes current state based on the given seed

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
	 */
	HighInterconnect(std::size_t width, bool randomta)
	: CoupledModel("High Interconnect")
	{
		std::vector<n_model::t_atomicmodelptr> ptrs;
		for(std::size_t i = 0; i < width; ++i){
			n_model::t_atomicmodelptr pt = n_tools::createObject<Generator>(std::string("Generator") + n_tools::toString(i), 1000*i, randomta);
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
