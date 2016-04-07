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


std::size_t getRand(std::size_t event, t_randgen& randgen)
{
    std::uniform_int_distribution<std::size_t> dist(0, 60000);
    randgen.seed(event);
    return dist(randgen);
}


/*
 * PHOLDTreeProcessor
 */

PHOLDTreeProcessor::PHOLDTreeProcessor(std::string name, size_t modelNumber, double percentagePriority)
    : AtomicModel(name), m_percentagePriority(percentagePriority), m_messageCount(0)
{
    state().m_events.push_back(EventPair(modelNumber, getProcTime(modelNumber)));
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

EventTime PHOLDTreeProcessor::getProcTime(EventTime event) const
{
    std::uniform_real_distribution<double> dist0(0.0, 1.0);
#ifdef FPTIME
    std::uniform_real_distribution<EventTime> dist(T_100, T_125);
    m_rand.seed(event);
    EventTime ta = roundTo(dist(m_rand), T_STEP);
#else
    std::uniform_int_distribution<EventTime> dist(T_100, T_125);
    m_rand.seed(event);
    EventTime ta = dist(m_rand);
#endif
    if(dist0(m_rand) < m_percentagePriority)
            return T_0;
    else
            return ta;
}

size_t PHOLDTreeProcessor::getNextDestination(size_t event) const
{
    m_rand.seed(event);
    size_t chosen = m_distDest(m_rand);
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
    state().m_events.pop_front();

}

void PHOLDTreeProcessor::confTransition(const std::vector<n_network::t_msgptr> & message)
{
    LOG_INFO("[PHOLDTree] - ",getName()," does a CONFLUENT TRANSITION");
    if (!state().m_events.empty()) {
        state().m_events.pop_front();
    }
    for (auto& msg : message) {
        ++m_messageCount;
        size_t payload = n_network::getMsgPayload<size_t>(msg);
        state().m_events.push_back(EventPair(payload, getProcTime(payload)));
    }
    LOG_INFO("[PHOLDTree] - ",getName()," has received ",m_messageCount," messages in total.");
}

void PHOLDTreeProcessor::extTransition(const std::vector<n_network::t_msgptr>& message)
{
    LOG_INFO("[PHOLDTree] - ",getName()," does an EXTERNAL TRANSITION");
    if (!state().m_events.empty()) {
        state().m_events[0].m_procTime -= m_elapsed.getTime();
    }
    for (auto& msg : message) {
        ++m_messageCount;
        size_t payload = n_network::getMsgPayload<size_t>(msg);
        state().m_events.push_back(EventPair(payload, getProcTime(payload)));
    }
    LOG_INFO("[PHOLDTree] - ",getName()," has received ",m_messageCount," messages in total.");
}

void PHOLDTreeProcessor::output(std::vector<n_network::t_msgptr>& msgs) const
{
    LOG_INFO("[PHOLDTree] - ",getName()," produces OUTPUT");

    if (!state().m_events.empty()) {
        const EventPair& i = state().m_events[0];
        size_t dest = getNextDestination(i.m_modelNumber);
        size_t r = getRand(i.m_modelNumber, m_rand);
        LOG_INFO("[PHOLDTree] - ",getName()," invokes createMessages on ", dest, " with arg ", r);
        m_oPorts[dest]->createMessages(r, msgs);
        LOG_INFO("[PHOLDTree] - ",getName()," Ports created ", msgs.size(), " messages.");
    }else
        {
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


n_model::t_portptr PHOLDTree::startConnection()
{
    return addOutPort(n_tools::toString(m_oPorts.size()));
}
n_model::t_portptr PHOLDTree::endConnection()
{
    return addInPort(n_tools::toString(m_iPorts.size()));
}

std::size_t numCounter = 0u;

PHOLDTree::PHOLDTree(const PHOLDTreeConfig& config, std::size_t depth)
    : PHOLDTree(config, depth, numCounter)
{ }

PHOLDTree::PHOLDTree(const PHOLDTreeConfig& config, std::size_t depth, std::size_t& itemNum)
    : n_model::CoupledModel(std::string("PHOLDTree_") + n_tools::toString(itemNum++))
{
    //create main child
    auto mainChild = n_tools::createObject<PHOLDTreeProcessor>("Processor_" + n_tools::toString(itemNum),
                                                             itemNum, config.percentagePriority);

    addSubModel(mainChild);
    ++itemNum;

    if(depth == 0) {
        //create all simple children
        for(std::size_t i = 0; i < config.numChildren; ++i) {
            auto itemPtr = n_tools::createObject<PHOLDTreeProcessor>("Processor_" + n_tools::toString(itemNum),
                                                                     itemNum, config.percentagePriority);
            ++itemNum;
            addSubModel(itemPtr);
            //connect main & child
        }
        //interconnect all children
        for(std::size_t i = 0; i < config.numChildren + 1; ++i) {
            for(std::size_t j = 0; j < config.numChildren + 1; ++j) {
                if(i == j) continue;
                auto ptr1 = std::static_pointer_cast<PHOLDTreeProcessor>(m_components[i])->startConnection();
                auto ptr2 = std::static_pointer_cast<PHOLDTreeProcessor>(m_components[j])->endConnection();
                connectPorts(ptr1, ptr2);
            }
        }
    } else {
        //create all simple children
        for(std::size_t i = 0; i < config.numChildren; ++i) {
            auto itemPtr = n_tools::createObject<PHOLDTree>(config, depth - 1, itemNum);
            ++itemNum;
            addSubModel(itemPtr);
            //connect main & child
            auto ptr1a = std::static_pointer_cast<PHOLDTree>(itemPtr)->startConnection();
            auto ptr2b = std::static_pointer_cast<PHOLDTree>(itemPtr)->endConnection();
            auto ptr2a = std::static_pointer_cast<PHOLDTreeProcessor>(mainChild)->startConnection();
            auto ptr1b = std::static_pointer_cast<PHOLDTreeProcessor>(mainChild)->endConnection();
            connectPorts(ptr1a, ptr1b);  // other to main child
            connectPorts(ptr2a, ptr2b);  // main to other child
        }
        //interconnect all children
        for(std::size_t i = 0; i < config.numChildren; ++i) {
            for(std::size_t j = 0; j < config.numChildren; ++j) {
                if(i == j) continue;
                // + 1 as the first child is the main child, which is not a PHOLDTree!
                auto ptr1 = std::static_pointer_cast<PHOLDTree>(m_components[i+1])->startConnection();
                auto ptr2 = std::static_pointer_cast<PHOLDTree>(m_components[j+1])->endConnection();
                connectPorts(ptr1, ptr2);
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
}

PHOLDTree::~PHOLDTree()
{ }

} /* namespace n_benchmarks_pholdtree */

