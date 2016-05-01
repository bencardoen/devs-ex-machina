/*
 * This file is part of the DEVS Ex Machina project.
 * Copyright 2014 - 2015 University of Antwerp
 * https://www.uantwerpen.be/en/
 * Licensed under the EUPL V.1.1
 * A full copy of the license is in COPYING.txt, or can be found at
 * https://joinup.ec.europa.eu/community/eupl/og_page/eupl
 *      Author: Stijn Manhaeve, Ben Cardoen
 */

#include <performance/pholdtree/pholdtree.h>
#include <limits>
#include <cmath>

#ifdef FPTIME
#define T_0 0.01    //timeadvance may NEVER be 0!
#define T_STEP 0.01
#define T_100 1.0
#define T_125 1.25
#else
#define T_0 1
#define T_STEP 1
#define T_100 100
#define T_125 125
#endif

namespace n_benchmarks_pholdtree
{
//assume we won't create a pholdtree model with 2^32+ connections
constexpr std::size_t nullDestination = std::numeric_limits<std::size_t>::max();

std::size_t getRand(std::size_t, t_randgen& randgen)
{
//    std::uniform_int_distribution<std::size_t> dist(0, 60000);
//    randgen.seed(event);
//    return dist(randgen);
    return randgen();
}


/*
 * PHOLDTreeProcessor
 */

PHOLDTreeProcessor::PHOLDTreeProcessor(std::string name, size_t modelNumber, double percentagePriority, size_t startSeed, bool isRoot)
    : AtomicModel(name), m_percentagePriority(percentagePriority), m_isRoot(isRoot), m_modelNumber(modelNumber)
{
    if(isRoot) {
        state().m_events.push_back(EventPair(modelNumber, getProcTime(modelNumber)));
    }
    state().m_rand.seed(startSeed);
}

PHOLDTreeProcessor::~PHOLDTreeProcessor()
{
}

template<typename T>
constexpr T roundTo(T val, T gran)
{
#ifdef __CYGWIN__
    return round(val/gran)*gran;
#else
    return std::round(val/gran)*gran;
#endif
}

EventTime PHOLDTreeProcessor::getProcTime(EventTime)
{
    //state().m_rand.seed(event);
    std::uniform_real_distribution<double> dist0(0.0, 1.0);
#ifdef FPTIME
    std::uniform_real_distribution<EventTime> dist(T_100, T_125);
    m_rand.seed(event);
    EventTime ta = roundTo(dist(m_rand), T_STEP);
#else
    std::uniform_int_distribution<EventTime> dist(T_100, T_125);
    EventTime ta = dist(state().m_rand);
#endif
    if(dist0(state().m_rand) < m_percentagePriority)
            return T_0;
    else
            return ta;
}

size_t PHOLDTreeProcessor::getNextDestination(size_t) const
{
    //std::cerr << "seed = " << event <<  "\n";
    //state().m_rand.seed(event);
    if(m_oPorts.empty())
        return nullDestination;
    size_t chosen = m_distDest(state().m_rand);
    //std::cerr << m_modelNumber << " choosing from [" << m_distDest.min() << ", " << m_distDest.max() << "] = " << chosen << "\n";
    LOG_INFO("[PHOLDTree] - Picked local: ", chosen);
    return chosen;
}

n_model::t_timestamp PHOLDTreeProcessor::timeAdvance() const
{
    LOG_INFO("[PHOLDTree] - ",getName()," does TIMEADVANCE");
    if (!state().m_events.empty()) {
        LOG_INFO("[PHOLDTree] - ",getName()," has an event with time ", state().m_events[0].m_procTime,".");
        return n_network::t_timestamp(state().m_events[0].m_procTime, 0);
    } else {
        LOG_INFO("[PHOLDTree] - ",getName()," has no events left, advances to infinity.");
        return n_network::t_timestamp::infinity();
    }
}

void PHOLDTreeProcessor::intTransition()
{
    LOG_INFO("[PHOLDTree] - ",getName()," does an INTERNAL TRANSITION");
#ifdef SAFETY_CHECKS
        if(state().m_events.size()==0)
                throw std::out_of_range("Int Transition pop on empty.");
#endif

        if(m_isRoot && state().m_events.size() == 1){
//            size_t seed = state().m_events.front().m_procTime * (m_modelNumber + 1);
            state().m_eventsProcessed++;
            state().m_events.push_back(EventPair(m_modelNumber, getProcTime(0)));
        }
        state().m_events.pop_front();

        if(state().m_events.size()) {
            finalize();
        }
}

void PHOLDTreeProcessor::confTransition(const std::vector<n_network::t_msgptr> & message)
{
    LOG_INFO("[PHOLDTree] - ",getName()," does a CONFLUENT TRANSITION");
    bool wasEmpty = state().m_events.empty();
    if (!wasEmpty) {
        state().m_events.pop_front();
    }
    for (auto& msg : message) {
        state().m_eventsProcessed++;
//        size_t payload = n_network::getMsgPayload<size_t>(msg)  + state().m_eventsProcessed;
        state().m_events.push_back(EventPair(m_modelNumber, getProcTime(0)));
    }
    if(wasEmpty) {
        finalize();
    }
    LOG_INFO("[PHOLDTree] - ",getName()," has received ",state().m_eventsProcessed," messages in total.");
}

void PHOLDTreeProcessor::extTransition(const std::vector<n_network::t_msgptr>& message)
{
    LOG_INFO("[PHOLDTree] - ",getName()," does an EXTERNAL TRANSITION");
    bool wasEmpty = state().m_events.empty();
    if (!wasEmpty) {
        state().m_events[0].m_procTime -= m_elapsed.getTime();
    }
    for (auto& msg : message) {
        state().m_eventsProcessed++;
//        size_t payload = n_network::getMsgPayload<size_t>(msg)  + state().m_eventsProcessed;
        state().m_events.push_back(EventPair(m_modelNumber, getProcTime(0)));
    }
    if(wasEmpty) {
        finalize();
    }
    LOG_INFO("[PHOLDTree] - ",getName()," has received ",state().m_eventsProcessed," messages in total.");
}

void PHOLDTreeProcessor::output(std::vector<n_network::t_msgptr>& msgs) const
{
    LOG_INFO("[PHOLDTree] - ",getName()," produces OUTPUT");

    if (!state().m_events.empty()) {
        const EventPair& i = state().m_events[0];
        size_t dest = state().m_destination;
        //don't do anything if the destination is the nulldestination
        if(dest == nullDestination)
            return;
        size_t r = state().m_nextMessage;
        LOG_INFO("[PHOLDTree] - ",getName()," invokes createMessages on ", dest, " with arg ", r);
        m_oPorts[dest]->createMessages(r, msgs);
        LOG_INFO("[PHOLDTree] - ",getName()," Ports created ", msgs.size(), " messages.");
    } else {
        LOG_WARNING("[PHOLDTree] - ",getName()," no events on state ?!");
    }
}


n_network::t_timestamp PHOLDTreeProcessor::lookAhead() const
{
    return T_STEP;
}
n_model::t_portptr PHOLDTreeProcessor::startConnection()
{
    n_model::t_portptr retVal = addOutPort(n_tools::toString(m_oPorts.size()));
    m_distDest = std::uniform_int_distribution<std::size_t>(0, m_oPorts.size()-1u);
    return retVal;
}
n_model::t_portptr PHOLDTreeProcessor::endConnection()
{
    return addInPort(n_tools::toString(m_iPorts.size()));
}

void PHOLDTreeProcessor::finalize()
{
    if(state().m_events.size()) {
        //schedule the very first item, otherwise, don't schedule anything
        state().m_destination = getNextDestination(state().m_events.front().m_modelNumber);
        state().m_nextMessage = getRand(state().m_events.front().m_modelNumber, state().m_rand);
    }
}


n_model::t_portptr PHOLDTree::startConnection()
{
    return addOutPort(n_tools::toString(m_oPorts.size()));
}
n_model::t_portptr PHOLDTree::endConnection()
{
    return addInPort(n_tools::toString(m_iPorts.size()));
}

PHOLDTree::PHOLDTree(PHOLDTreeConfig& config)
    : PHOLDTree(config, config.depth, config.numCounter)
{ }

PHOLDTree::PHOLDTree(PHOLDTreeConfig& config, std::size_t depth, std::size_t& itemNum)
    : n_model::CoupledModel(std::string("PHOLDTree_") + n_tools::toString(itemNum++))
{
    //create main child
    bool isRootSpawn = (depth == config.depth) && config.spawnAtRoot;
    if(isRootSpawn)
        config.getSeed.seed(config.initialSeed);
    auto mainChild = n_tools::createObject<PHOLDTreeProcessor>("Processor_" + n_tools::toString(itemNum),
                                                             itemNum, config.percentagePriority, config.getSeed(), isRootSpawn);

    addSubModel(mainChild);
    ++itemNum;
    std::size_t counterMax = config.numChildren + (config.circularLinks? 0:-1);
    
    if(depth == 0) {
        //create all simple children
        for(std::size_t i = 0; i < config.numChildren; ++i) {
            auto itemPtr = n_tools::createObject<PHOLDTreeProcessor>("Processor_" + n_tools::toString(itemNum),
                                                                     itemNum, config.percentagePriority, config.getSeed());
            ++itemNum;
            addSubModel(itemPtr);
            //connect main & child
        }
        //interconnect all children
        for(std::size_t i = 0; i < counterMax; ++i) {
            auto ptr1a = std::static_pointer_cast<PHOLDTreeProcessor>(m_components[i+1])->startConnection();
            auto ptr2a = std::static_pointer_cast<PHOLDTreeProcessor>(m_components[(i+1)%config.numChildren + 1])->endConnection();
            connectPorts(ptr1a, ptr2a);
            if(config.doubleLinks){
                auto ptr1b = std::static_pointer_cast<PHOLDTreeProcessor>(m_components[(i+1)%config.numChildren +1])->startConnection();
                auto ptr2b = std::static_pointer_cast<PHOLDTreeProcessor>(m_components[i+1])->endConnection();
                connectPorts(ptr1b, ptr2b);
            }
        }
        //connect main child with other children
        for(std::size_t i = 0; i < config.numChildren; ++i) {
            auto ptr1a = mainChild->startConnection();
            auto ptr2a = std::static_pointer_cast<PHOLDTreeProcessor>(m_components[i+1])->endConnection();
            connectPorts(ptr1a, ptr2a);
            if(config.doubleLinks){
                auto ptr1b = std::static_pointer_cast<PHOLDTreeProcessor>(m_components[i+1])->startConnection();
                auto ptr2b = mainChild->endConnection();
                connectPorts(ptr1b, ptr2b);
            }
            std::static_pointer_cast<PHOLDTreeProcessor>(m_components[i+1])->finalize();
        }
    } else {
        //create all recursive children
        for(std::size_t i = 0; i < config.numChildren; ++i) {
            auto itemPtr = n_tools::createObject<PHOLDTree>(config, depth - 1, itemNum);
            ++itemNum;
            addSubModel(itemPtr);
            //connect main & child
            auto ptr1a = std::static_pointer_cast<PHOLDTreeProcessor>(mainChild)->startConnection();
            auto ptr1b = std::static_pointer_cast<PHOLDTree>(itemPtr)->endConnection();
            connectPorts(ptr1a, ptr1b);  // main to other child
            if(config.doubleLinks){
                auto ptr2a = std::static_pointer_cast<PHOLDTree>(itemPtr)->startConnection();
                auto ptr2b = std::static_pointer_cast<PHOLDTreeProcessor>(mainChild)->endConnection();
                connectPorts(ptr2a, ptr2b);  // other to main child
            }
        }
        //interconnect all children
        for(std::size_t i = 0; i < counterMax; ++i) {
            // + 1 as the first child is the main child, which is not a PHOLDTree!
            auto ptr1a = std::static_pointer_cast<PHOLDTree>(m_components[i+1])->startConnection();
            auto ptr2a = std::static_pointer_cast<PHOLDTree>(m_components[(i+1)%config.numChildren + 1])->endConnection();
            connectPorts(ptr1a, ptr2a);
            if(config.doubleLinks){
                auto ptr1b = std::static_pointer_cast<PHOLDTree>(m_components[(i+1)%config.numChildren +1])->startConnection();
                auto ptr2b = std::static_pointer_cast<PHOLDTree>(m_components[i+1])->endConnection();
                connectPorts(ptr1b, ptr2b);
            }
        }
        //finalize subcomponents
        for(std::size_t i = 0; i < config.numChildren; ++i) {
            // + 1 as the first child is the main child, which is not a PHOLDTree!
            std::static_pointer_cast<PHOLDTree>(m_components[i+1])->finalizeSetup();
        }
    }

}
void PHOLDTree::finalizeSetup()
{
    std::shared_ptr<PHOLDTreeProcessor> mainChild = std::static_pointer_cast<PHOLDTreeProcessor>(m_components[0]);
    //connect main child to all my ports
    for(auto& port: m_oPorts) {
        auto ptr1 = mainChild->startConnection();
        connectPorts(ptr1, port);
    }
    for(auto& port: m_iPorts) {
        auto ptr1 = mainChild->endConnection();
        connectPorts(port, ptr1);
    }
    mainChild->finalize();
}

PHOLDTree::~PHOLDTree()
{ }

void allocateTree(std::shared_ptr<PHOLDTree>& root, const PHOLDTreeConfig& config, std::size_t numCores) {
    //precompute number of items per core
    if(numCores < 2) return; // no multithreading, all is automatically allocated on one core.
    const std::size_t p = config.numChildren;
    const std::size_t d = config.depth+2;   //+ 2 because the tree is actually 2 layers larger because we put a PHOLDTree at level 0
    const std::size_t numItems = std::ceil(double(std::pow(p, d) - 1) / ((p - 1)*numCores));
    std::size_t curNumChildren = 0;
    int curCore = 0;
    //need breadth-first search through the entire tree
    std::deque<n_model::t_modelptr> todoList;
    todoList.push_back(std::static_pointer_cast<n_model::t_modelptr::element_type>(root));
    while(todoList.size()) {
        //get the top item
        n_model::t_modelptr top = todoList.front();
        todoList.pop_front();
        //test if it is a PHOLDTree item
        auto treeTop = std::dynamic_pointer_cast<PHOLDTree>(top);
        if(treeTop != nullptr) {
            const auto& components  = treeTop->getComponents();
            if(config.depthFirstAlloc) {
                //just add everything in reverse order to the front of the todo list
                todoList.insert(todoList.begin(), components.rbegin(), components.rend());
            } else {
                //add the main child of the item
                auto mnChild = std::static_pointer_cast<PHOLDTreeProcessor>(components[0]);
                mnChild->setCorenumber(curCore);
#ifndef BENCHMARK
                std::cerr << "allocating " << mnChild->getName() << " to " << curCore << "\n";
#endif
                LOG_DEBUG("allocating ", mnChild->getName(), " to ", curCore);
                ++curNumChildren;
                if(curNumChildren == numItems) {
                    ++curCore;
                    curNumChildren = 0;
                }
                //add the other children to the todoList
                todoList.insert(todoList.end(), components.begin()+1, components.end());
            }
        } else if(!config.depthFirstAlloc){
            //from here on, everything must be a normal PHOLDTreeProcessor in breadth first search.
            todoList.push_front(top);
            while(todoList.size()) {
                n_model::t_modelptr itop = todoList.front();
                todoList.pop_front();
                auto procItem = std::static_pointer_cast<PHOLDTreeProcessor>(itop);
                procItem->setCorenumber(curCore);
#ifndef BENCHMARK
                std::cerr << "allocating " << procItem->getName() << " to " << curCore << "\n";
#endif
                LOG_DEBUG("allocating ", procItem->getName(), " to ", curCore);
                ++curNumChildren;
                if(curNumChildren == numItems) {
                    ++curCore;
                    curNumChildren = 0;
                }
            }
        } else {
            //depth first search encounter of a PHOLDTreeProcessor
            auto procItem = std::static_pointer_cast<PHOLDTreeProcessor>(top);
            procItem->setCorenumber(curCore);
#ifndef BENCHMARK
            std::cerr << "allocating " << procItem->getName() << " to " << curCore << "\n";
#endif
            LOG_DEBUG("allocating ", procItem->getName(), " to ", curCore);
            ++curNumChildren;
            if(curNumChildren == numItems) {
                ++curCore;
                curNumChildren = 0;
            }
        }
    }
}

} /* namespace n_benchmarks_pholdtree */

