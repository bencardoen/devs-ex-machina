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

std::shared_ptr<PHOLDModelState> PHOLDModelState::copy()
{
	auto newState = n_tools::createObject<PHOLDModelState>();
	newState->m_events = m_events;
	return newState;
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
		addOutPort("outport_" + n_tools::toString(i));
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
	return rand() % 10; //TODO determine proper rand value, since we don't use floats
}

size_t HeavyPHOLDProcessor::getNextDestination(size_t event) const
{
	srand(event);
	if ((static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) > m_percentageRemotes || m_remote.empty()) {
		return m_local[rand() % m_local.size()];
	} else {
		return m_remote[rand() % m_remote.size()];
	}
}

n_model::t_timestamp HeavyPHOLDProcessor::timeAdvance() const
{
	std::cout<<"ADVANCE "<<m_elapsed<<std::endl;
	std::shared_ptr<PHOLDModelState> state = std::dynamic_pointer_cast<PHOLDModelState>(getState());
	if (!state->m_events.empty()) {
		return n_network::t_timestamp(state->m_events[0].m_procTime, 0);
	} else {
		return n_network::t_timestamp::infinity();
	}
}

void HeavyPHOLDProcessor::intTransition()
{
	std::shared_ptr<PHOLDModelState> newState = std::dynamic_pointer_cast<PHOLDModelState>(getState())->copy();
	newState->m_events.pop_front();
	setState(newState);
}

void HeavyPHOLDProcessor::confTransition(const std::vector<n_network::t_msgptr> & message)
{
	std::shared_ptr<PHOLDModelState> newState = std::dynamic_pointer_cast<PHOLDModelState>(getState())->copy();
	if (!newState->m_events.empty()) {
		newState->m_events.pop_front();
	}
	for (auto& msg : message) {
		int payload = n_tools::toInt(msg->getPayload());
		newState->m_events.push_back(EventPair(payload, getProcTime(payload)));
		for (size_t i = 0; i < m_iter; ++i); 	// We just do stuff for a while
	}
	setState(newState);
}

void HeavyPHOLDProcessor::extTransition(const std::vector<n_network::t_msgptr>& message)
{
	std::shared_ptr<PHOLDModelState> newState = std::dynamic_pointer_cast<PHOLDModelState>(getState())->copy();
	if (!newState->m_events.empty()) {
		newState->m_events[0].m_procTime -= m_elapsed.getTime();
	}
	for (auto& msg : message) {
		int payload = n_tools::toInt(msg->getPayload());
		newState->m_events.push_back(EventPair(payload, getProcTime(payload)));
		for (size_t i = 0; i < m_iter; ++i); 	// We just do stuff for a while
	}
	setState(newState);
}

std::vector<n_network::t_msgptr> HeavyPHOLDProcessor::output() const
{
	std::shared_ptr<PHOLDModelState> state = std::dynamic_pointer_cast<PHOLDModelState>(getState())->copy();;

	if (!state->m_events.empty()) {
		srand(state->m_events[0].m_modelNumber);
		int r = rand() % 60000;
		size_t i = getNextDestination(state->m_events[0].m_modelNumber);
		return getPort("outport_" + n_tools::toString(i))->createMessages(n_tools::toString(r));
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
