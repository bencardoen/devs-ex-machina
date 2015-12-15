/*
 * This file is part of the DEVS Ex Machina project.
 * Copyright 2014 - 2015 University of Antwerp
 * https://www.uantwerpen.be/en/
 * Licensed under the EUPL V.1.1
 * A full copy of the license is in COPYING.txt, or can be found at
 * https://joinup.ec.europa.eu/community/eupl/og_page/eupl
 *      Author: Stijn Manhaeve, Ben Cardoen, Tim Tuijn, Matthijs Van Os
 */

#include <performance/phold/phold.h>
#include <random>

#ifdef FPTIME
#define T_0 0.01	//timeadvance may NEVER be 0!
#define T_STEP 0.01
#define T_100 1.0
#define T_125 1.25
#else
#define T_0 1
#define T_STEP 1
#define T_100 100
#define T_125 125
#endif

namespace n_benchmarks_phold {


std::size_t getRand(std::size_t event, t_randgen& randgen)
{
	std::uniform_int_distribution<std::size_t> dist(0, 60000);
	randgen.seed(event);
	return dist(randgen);
}


/*
 * HeavyPHOLDProcessor
 */

HeavyPHOLDProcessor::HeavyPHOLDProcessor(std::string name, size_t iter, size_t totalAtomics, size_t modelNumber,
        std::vector<size_t> local, std::vector<size_t> remote, size_t percentageRemotes, double percentagePriority)
	: AtomicModel(name), m_percentageRemotes(percentageRemotes), m_percentagePriority(percentagePriority), m_iter(iter), m_local(local), m_remote(remote), m_messageCount(0)
{
	addInPort("inport");
	for (size_t i = 0; i < totalAtomics; ++i) {
		m_outs.push_back(addOutPort("outport_" + n_tools::toString(i)));
	}
	state().m_events.push_back(EventPair(modelNumber, getProcTime(modelNumber)));
}

HeavyPHOLDProcessor::~HeavyPHOLDProcessor()
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

EventTime HeavyPHOLDProcessor::getProcTime(EventTime event) const
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

size_t HeavyPHOLDProcessor::getNextDestination(size_t event) const
{
	LOG_INFO("[PHOLD] - Getting next destination. [Local=",m_local.size(),"] [Remote=",m_remote.size(),"]");
	std::uniform_int_distribution<std::size_t> dist(0, 100);
	std::uniform_int_distribution<std::size_t> distRemote(0, m_remote.size()-1u);
	std::uniform_int_distribution<std::size_t> distLocal(0, m_local.size()-1u);
	m_rand.seed(event);
	if (dist(m_rand) > m_percentageRemotes || m_remote.empty()) {
                size_t rnr = distLocal(m_rand);
#ifdef SAFETY_CHECKS
                size_t chosen = m_local.at(rnr);
#else   
                size_t chosen = m_local[rnr];
#endif
		LOG_INFO("[PHOLD] - Picked local: ", chosen);
		return chosen;
	} else {
		size_t chosen = m_remote[distRemote(m_rand)];
		LOG_INFO("[PHOLD] - Picked remote: ", chosen);
		return chosen;
	}
}

n_model::t_timestamp HeavyPHOLDProcessor::timeAdvance() const
{
	LOG_INFO("[PHOLD] - ",getName()," does TIMEADVANCE");
	if (!state().m_events.empty()) {
		LOG_INFO("[PHOLD] - ",getName()," has an event with time ", state().m_events[0].m_procTime,".");
		return n_network::t_timestamp(state().m_events[0].m_procTime, 0);
	} else {
		LOG_INFO("[PHOLD] - ",getName()," has no events left, advances to infinity.");
		return n_network::t_timestamp::infinity();
	}
}

void HeavyPHOLDProcessor::intTransition()
{
	LOG_INFO("[PHOLD] - ",getName()," does an INTERNAL TRANSITION");
#ifdef SAFETY_CHECKS
        if(state().m_events.size()==0)
                throw std::out_of_range("Int Transition pop on empty.");
#endif
	state().m_events.pop_front();

}

void HeavyPHOLDProcessor::confTransition(const std::vector<n_network::t_msgptr> & message)
{
	LOG_INFO("[PHOLD] - ",getName()," does a CONFLUENT TRANSITION");
	if (!state().m_events.empty()) {
		state().m_events.pop_front();
	}
	for (auto& msg : message) {
		++m_messageCount;
		size_t payload = n_network::getMsgPayload<size_t>(msg);
		state().m_events.push_back(EventPair(payload, getProcTime(payload)));
		volatile size_t i = 0;
		for (; i < m_iter;){ ++i; } 	// We just do stuff for a while
	}
	LOG_INFO("[PHOLD] - ",getName()," has received ",m_messageCount," messages in total.");
}

void HeavyPHOLDProcessor::extTransition(const std::vector<n_network::t_msgptr>& message)
{
	LOG_INFO("[PHOLD] - ",getName()," does an EXTERNAL TRANSITION");
	if (!state().m_events.empty()) {
		state().m_events[0].m_procTime -= m_elapsed.getTime();
	}
	for (auto& msg : message) {
		++m_messageCount;
		size_t payload = n_network::getMsgPayload<size_t>(msg);
		state().m_events.push_back(EventPair(payload, getProcTime(payload)));
		volatile size_t i = 0;
		for (; i < m_iter;){ ++i; } 	// We just do stuff for a while
	}
	LOG_INFO("[PHOLD] - ",getName()," has received ",m_messageCount," messages in total.");
}

void HeavyPHOLDProcessor::output(std::vector<n_network::t_msgptr>& msgs) const
{
	LOG_INFO("[PHOLD] - ",getName()," produces OUTPUT");

	if (!state().m_events.empty()) {
		const EventPair& i = state().m_events[0];
		size_t dest = getNextDestination(i.m_modelNumber);
		size_t r = getRand(i.m_modelNumber, m_rand);
		LOG_INFO("[PHOLD] - ",getName()," invokes createMessages on ", dest, " with arg ", r);
		m_outs[dest]->createMessages(r, msgs);
		LOG_INFO("[PHOLD] - ",getName()," Ports created ", msgs.size(), " messages.");
	}else
        {
                LOG_WARNING("[PHOLD] - ",getName()," no events on state ?!");
        }
}


n_network::t_timestamp HeavyPHOLDProcessor::lookAhead() const
{
	return T_STEP;
}

/*
 * PHOLD
 */

PHOLD::PHOLD(size_t nodes, size_t atomicsPerNode, size_t iter, std::size_t percentageRemotes, double percentagePriority)
	: n_model::CoupledModel("PHOLD")
{
	std::vector<n_model::t_atomicmodelptr> processors;
	std::vector<std::vector<size_t>> procs;

	size_t totalAtomics = nodes * atomicsPerNode;

	for (size_t i = 0; i < nodes; ++i) {
		procs.push_back(std::vector<size_t>());
		for (size_t j = 0; j < atomicsPerNode; ++j) {
			procs[i].push_back(atomicsPerNode * i + j);
		}
	}

	size_t cntr = 0;
	for (size_t i = 0; i < nodes; ++i) {
		std::vector<size_t> allnoi;
		for (size_t k = 0; k < nodes; ++k) {
			if (i != k)
				allnoi.insert(allnoi.end(), procs[k].begin(), procs[k].end());
		}
		for (size_t num : procs[i]) {
			std::vector<size_t> inoj = procs[i];
			inoj.erase(std::remove(inoj.begin(), inoj.end(), num), inoj.end());
			auto p = n_tools::createObject<HeavyPHOLDProcessor>("Processor_" + n_tools::toString(cntr),
			        iter, totalAtomics, cntr, inoj, allnoi, percentageRemotes, percentagePriority);
			processors.push_back(p);
			addSubModel(p);
			++cntr;
		}
	}

	for (size_t i = 0; i < processors.size(); ++i) {
		for (size_t j = 0; j < processors.size(); ++j) {
			if (i == j)
				continue;
			connectPorts(processors[i]->getOPorts()[j],
			        processors[j]->getIPorts()[0]);
		}
	}
}

PHOLD::~PHOLD()
{
}

} /* namespace n_benchmarks_phold */
