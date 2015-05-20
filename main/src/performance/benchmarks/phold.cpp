/*
 * phold.cpp
 *
 *  Created on: 18 May 2015
 *      Author: matthijs
 */

#include <performance/benchmarks/phold.h>

namespace n_benchmarks_phold {

/*
 * PHOLDModelState
 */

PHOLDModelState::PHOLDModelState()
{
}

PHOLDModelState::~PHOLDModelState()
{
}


/*
 * HeavyPHOLDProcessor
 */

HeavyPHOLDProcessor::HeavyPHOLDProcessor(std::string name, size_t iter, size_t totalAtomics, size_t modelNumber,
        std::vector<size_t> local, std::vector<size_t> remote, size_t percentageRemotes)
	: AtomicModel(name), m_percentageRemotes(percentageRemotes), m_totalAtomics(totalAtomics),
	  m_modelNumber(modelNumber), m_iter(iter), m_local(local), m_remote(remote)
{
	addInPort("inport");
	for (size_t i = 0; i < totalAtomics; ++i) {
		m_outs.push_back(addOutPort("outport_" + n_tools::toString(i)));
	}
	auto state = n_tools::createObject<PHOLDModelState>();
	state->m_events.push_back(EventPair(modelNumber, getProcTime(modelNumber)));
	setState(state);
}

HeavyPHOLDProcessor::~HeavyPHOLDProcessor()
{
}

size_t HeavyPHOLDProcessor::getProcTime(size_t event) const
{
	srand(event);
	return rand() % 101; //TODO determine proper rand value, since we don't use floats
}

size_t HeavyPHOLDProcessor::getNextDestination(size_t event) const
{
	LOG_INFO("[PHOLD] - Getting next destination. [Local=",m_local.size(),"] [Remote=",m_remote.size(),"]");
	srand(event);
	if (rand() % 101 > (int)(m_percentageRemotes*100) || m_remote.empty()) {
		size_t chosen = m_local[rand() % m_local.size()];
		LOG_INFO("[PHOLD] - Picked local: ", chosen);
		return chosen;
	} else {
		size_t chosen = m_remote[rand() % m_remote.size()];
		LOG_INFO("[PHOLD] - Picked remote: ", chosen);
		return chosen;
	}
}

n_model::t_timestamp HeavyPHOLDProcessor::timeAdvance() const
{
	LOG_INFO("[PHOLD] - ",getName()," does TIMEADVANCE");
	std::shared_ptr<PHOLDModelState> state = std::dynamic_pointer_cast<PHOLDModelState>(getState());
	if (!state->m_events.empty()) {
		LOG_INFO("[PHOLD] - ",getName()," has an event with time ",state->m_events[0].m_procTime,".");
		return n_network::t_timestamp(state->m_events[0].m_procTime, 0);
	} else {
		LOG_INFO("[PHOLD] - ",getName()," has no events left, advances to infinity.");
		return n_network::t_timestamp::infinity();
	}
}

void HeavyPHOLDProcessor::intTransition()
{
	LOG_INFO("[PHOLD] - ",getName()," does an INTERNAL TRANSITION");
	std::shared_ptr<PHOLDModelState> newState = n_tools::createObject<PHOLDModelState>(
		        *std::dynamic_pointer_cast<PHOLDModelState>(getState()));
	newState->m_events.pop_front();
	setState(newState);
}

void HeavyPHOLDProcessor::confTransition(const std::vector<n_network::t_msgptr> & message)
{
	LOG_INFO("[PHOLD] - ",getName()," does a CONFLUENT TRANSITION");
	std::shared_ptr<PHOLDModelState> newState = n_tools::createObject<PHOLDModelState>(
		        *std::dynamic_pointer_cast<PHOLDModelState>(getState()));
	if (!newState->m_events.empty()) {
		newState->m_events.pop_front();
	}
	for (auto& msg : message) {
		++m_messageCount;
		size_t payload = n_network::getMsgPayload<size_t>(msg);
		newState->m_events.push_back(EventPair(payload, getProcTime(payload)));
		std::this_thread::sleep_for(std::chrono::milliseconds(m_iter)); // Wait a bit.
	}
	LOG_INFO("[PHOLD] - ",getName()," has received ",m_messageCount," messages in total.");
	setState(newState);
}

void HeavyPHOLDProcessor::extTransition(const std::vector<n_network::t_msgptr>& message)
{
	LOG_INFO("[PHOLD] - ",getName()," does an EXTERNAL TRANSITION");
	std::shared_ptr<PHOLDModelState> newState = n_tools::createObject<PHOLDModelState>(
	        *std::dynamic_pointer_cast<PHOLDModelState>(getState()));
	if (!newState->m_events.empty()) {
		newState->m_events[0].m_procTime -= m_elapsed.getTime();
	}
	for (auto& msg : message) {
		++m_messageCount;
		size_t payload = n_network::getMsgPayload<size_t>(msg);
		newState->m_events.push_back(EventPair(payload, getProcTime(payload)));
		for (size_t i = 0; i < m_iter; ++i); 	// We just do stuff for a while
	}
	LOG_INFO("[PHOLD] - ",getName()," has received ",m_messageCount," messages in total.");
	setState(newState);
}

std::vector<n_network::t_msgptr> HeavyPHOLDProcessor::output() const
{
	LOG_INFO("[PHOLD] - ",getName()," produces OUTPUT");
	std::shared_ptr<PHOLDModelState> state = std::dynamic_pointer_cast<PHOLDModelState>(getState());

	if (!state->m_events.empty()) {
		EventPair& i = state->m_events[0];
		srand(i.m_modelNumber);
		size_t dest = getNextDestination(i.m_modelNumber);
		size_t r = rand() % 60000;
		LOG_INFO("[PHOLD] - ",getName()," sends a message to outport ", dest);
		return m_outs[dest]->createMessages(r);
	}
	return std::vector<n_network::t_msgptr>();
}

/*
 * PHOLD
 */

PHOLD::PHOLD(size_t nodes, size_t atomicsPerNode, size_t iter, float percentageRemotes)
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
			        iter, totalAtomics, cntr, inoj, allnoi, percentageRemotes);
			processors.push_back(p);
			addSubModel(p);
			++cntr;
		}
	}

	for (size_t i = 0; i < processors.size(); ++i) {
		for (size_t j = 0; j < processors.size(); ++j) {
			if (i == j)
				continue;
			connectPorts(processors[i]->getPort("outport_" + n_tools::toString(i)),
			        processors[j]->getPort("inport"));
		}
	}
}

PHOLD::~PHOLD()
{
}

} /* namespace n_benchmarks_phold */
