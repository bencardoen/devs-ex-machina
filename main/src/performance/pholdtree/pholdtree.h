/*
 * This file is part of the DEVS Ex Machina project.
 * Copyright 2014 - 2015 University of Antwerp
 * https://www.uantwerpen.be/en/
 * Licensed under the EUPL V.1.1
 * A full copy of the license is in COPYING.txt, or can be found at
 * https://joinup.ec.europa.eu/community/eupl/og_page/eupl
 *      Author: Stijn Manhaeve, Ben Cardoen
 */

#ifndef SRC_PERFORMANCE_PHOLDTREE_PHOLDTREE_H_
#define SRC_PERFORMANCE_PHOLDTREE_PHOLDTREE_H_

#include <stdlib.h>
#include <thread>
#include <chrono>
#include <random>
#include <cmath>
#include <cstdlib>
#include <cinttypes>
#include "model/atomicmodel.h"
#include "model/coupledmodel.h"
#include <boost/random.hpp>
#include "control/allocator.h"
#include "tools/frandom.h"

namespace n_benchmarks_pholdtree
{

// pholdtree uses event counters just like phold.
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

struct PHOLDTreeModelState
{
    std::deque<EventPair> m_events;
    std::size_t m_eventsProcessed;

    PHOLDTreeModelState(): m_eventsProcessed(0) { }
};

} /* namespace n_benchmarks_pholdtree */

template<>
struct ToString<n_benchmarks_pholdtree::PHOLDTreeModelState>
{
    static std::string exec(const n_benchmarks_pholdtree::PHOLDTreeModelState&){
        return "";
    }
};

namespace n_benchmarks_pholdtree {


//typedef boost::random::taus88 t_randgen;
//typedef boost::random::mt11213b t_randgen;
//typedef n_tools::n_frandom::marsaglia_xor_64_s t_randgen;
//typedef std::mt19937_64 t_randgen;
//typedef n_tools::n_frandom::t_fastrng t_randgen;
#ifdef FRNG
    typedef n_tools::n_frandom::t_fastrng t_randgen;
#else
    typedef std::mt19937_64 t_randgen;
#endif

class PHOLDTreeProcessor: public n_model::AtomicModel<PHOLDTreeModelState>
{
private:
    mutable std::uniform_int_distribution<std::size_t> m_distDest;
    const double m_percentagePriority;
    mutable t_randgen m_rand;   //This object could be a global object, but then we'd need to lock it during parallel simulation.
    bool m_isRoot;
    std::size_t m_modelNumber;
public:
    PHOLDTreeProcessor(std::string name, size_t modelNumber, double percentagePriority, bool isRoot = false);
    virtual ~PHOLDTreeProcessor();

    virtual n_network::t_timestamp timeAdvance() const override;
    virtual void intTransition() override;
    virtual void confTransition(const std::vector<n_network::t_msgptr> & message) override;
    virtual void extTransition(const std::vector<n_network::t_msgptr> & message) override;
    virtual void output(std::vector<n_network::t_msgptr>& msgs) const override;
    virtual n_network::t_timestamp lookAhead() const override;

    // Adds an output port for a new connection & returns it
    n_model::t_portptr startConnection();
    // Adds an input port for a new connection & returns it
    n_model::t_portptr endConnection();


    EventTime getProcTime(size_t event) const;
    size_t getNextDestination(size_t event) const;
};

struct PHOLDTreeConfig
{
    std::size_t numChildren;    //number of children per node
    std::size_t depth;          //depth of the tree
    double percentagePriority;  //priority message spawn chance
    bool spawnAtRoot;           //only root node can spawn
    bool doubleLinks;           //make links double
    bool circularLinks;         //make children a circular linked list
    //other configuration?
    PHOLDTreeConfig(): numChildren(0u), depth(0), percentagePriority(0.1), spawnAtRoot(true),
                        doubleLinks(false), circularLinks(false)
    {}
};

class PHOLDTree: public n_model::CoupledModel
{
private:
    void finalizeSetup();
public:
    PHOLDTree(const PHOLDTreeConfig& config, std::size_t depth, std::size_t& itemNum);
    PHOLDTree(const PHOLDTreeConfig& config);
    virtual ~PHOLDTree();

    // Adds an output port for a new connection & returns it
    n_model::t_portptr startConnection();
    // Adds an input port for a new connection & returns it
    n_model::t_portptr endConnection();
};

class PHoldTreeAlloc: public n_control::Allocator
{
private:
    // len(models);
    std::size_t m_maxn;
    // Last alloc id
    std::size_t m_n;
    // Bucketsize (ceiled)
    std::size_t m_nodes_per_core;

public:
        PHoldTreeAlloc(): m_maxn(0), m_n(0), m_nodes_per_core(0)
    {
    }
    virtual size_t allocate(const n_model::t_atomicmodelptr&){

        return (m_n++)%coreAmount();
    }

    virtual void allocateAll(const std::vector<n_model::t_atomicmodelptr>& models){
        //temporary implementation
        //better implementations are to come
        /**
         *  Given that we have N models, C simulation cores and Phold : #nodes = A, #apn=S.
         *  We want to allocate each N's submodels to exactly 1 core.
         *  Ideally this would correspond with a the N identifier for the subnodes, but we don't seem
         *  to store this so distribute the list of generated models in stripes of length (size/cores).
         *  If A does not match C, it is useless to run phold. (R is void of any meaning then).
         */
        for(auto& i: models)
            i->setCorenumber(allocate(i));
    }
};

} /* namespace n_benchmarks_pholdtree */

#endif /* SRC_PERFORMANCE_PHOLDTREE_PHOLDTREE_H_ */
