/*
 * This file is part of the DEVS Ex Machina project.
 * Copyright 2014 - 2015 University of Antwerp
 * https://www.uantwerpen.be/en/
 * Licensed under the EUPL V.1.1
 * A full copy of the license is in COPYING.txt, or can be found at
 * https://joinup.ec.europa.eu/community/eupl/og_page/eupl
 *      Author: Stijn Manhaeve, Ben Cardoen, Tim Tuijn, Matthijs Van Os
 */

#ifndef SRC_PERFORMANCE_PHOLD_PHOLD_H_
#define SRC_PERFORMANCE_PHOLD_PHOLD_H_

#include <stdlib.h>
#include <thread>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <cinttypes>
#include "model/atomicmodel.h"
#include "model/coupledmodel.h"
#include "control/allocator.h"

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

struct PHOLDModelState
{
	std::deque<EventPair> m_events;
};

}	/* namespace n_benchmarks_phold */

template<>
struct ToString<n_benchmarks_phold::PHOLDModelState>
{
	static std::string exec(const n_benchmarks_phold::PHOLDModelState&){
		return "";
	}
};

namespace n_benchmarks_phold {

typedef std::mt19937_64 t_randgen;	//don't use the default one. It's not random enough.

class HeavyPHOLDProcessor: public n_model::AtomicModel<PHOLDModelState>
{
private:
	const size_t m_percentageRemotes;
	const double m_percentagePriority;
	const size_t m_iter;
	std::vector<size_t> m_local;
	std::vector<size_t> m_remote;
	int m_messageCount;
	std::vector<n_model::t_portptr> m_outs;
	mutable t_randgen m_rand;	//This object could be a global object, but then we'd need to lock it during parallel simulation.
public:
	HeavyPHOLDProcessor(std::string name, size_t iter, size_t totalAtomics, size_t modelNumber, std::vector<size_t> local,
	        std::vector<size_t> remote, size_t percentageRemotes, double percentagePriority);
	virtual ~HeavyPHOLDProcessor();

	virtual n_network::t_timestamp timeAdvance() const override;
	virtual void intTransition() override;
	virtual void confTransition(const std::vector<n_network::t_msgptr> & message) override;
	virtual void extTransition(const std::vector<n_network::t_msgptr> & message) override;
	virtual void output(std::vector<n_network::t_msgptr>& msgs) const override;
	virtual n_network::t_timestamp lookAhead() const override;

	EventTime getProcTime(size_t event) const;
	size_t getNextDestination(size_t event) const;
};

class PHOLD: public n_model::CoupledModel
{
public:
	PHOLD(size_t nodes, size_t atomicsPerNode, size_t iter, std::size_t percentageRemotes, double percentagePriority = 0.1);
	virtual ~PHOLD();
};

class PHoldAlloc: public n_control::Allocator
{
private:
        // len(models);
	std::size_t m_maxn;
        // Last alloc id
	std::size_t m_n;
        // Bucketsize (ceiled)
        std::size_t m_nodes_per_core;
        
public:
	PHoldAlloc(): m_maxn(0), m_n(0), m_nodes_per_core(0)
	{
	}
	virtual size_t allocate(const n_model::t_atomicmodelptr&){
		
		return (m_n++)%coreAmount();
	}

	virtual void allocateAll(const std::vector<n_model::t_atomicmodelptr>& models){
        /**
         *  Given that we have N models, C simulation cores and Phold : #nodes = A, #apn=S.
         *  We want to allocate each N's submodels to exactly 1 core. 
         *  Ideally this would correspond with a the N identifier for the subnodes, but we don't seem
         *  to store this so distribute the list of generated models in stripes of length (size/cores).
         *  If A does not match C, it is useless to run phold. (R is void of any meaning then).
         */
		m_maxn = models.size();
                m_nodes_per_core = std::ceil(m_maxn / (double)coreAmount());
		assert(m_maxn && "Total amount of models can't be zero.");
		for(size_t i = 0; i< models.size(); ++i){
                        auto qr = lldiv(i, m_nodes_per_core);
						size_t coreid = qr.quot;
                        if(coreid >= coreAmount()){     // overflow into the last core.
                                coreid = coreAmount()-1;
                        }
			models[i]->setCorenumber(coreid);
			LOG_DEBUG("Assigning model ", models[i]->getName(), " to core ", coreid);
		}
	}
};

} /* namespace n_benchmarks_phold */

#endif /* SRC_PERFORMANCE_PHOLD_PHOLD_H_ */
