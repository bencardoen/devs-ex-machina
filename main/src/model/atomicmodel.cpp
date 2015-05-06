/*
 * atomicmodel.cpp
 *
 *  Created on: Mar 17, 2015
 *      Author: Tim Stijn
 */

#include "atomicmodel.h"

namespace n_model {

AtomicModel::AtomicModel(std::string name, std::size_t)
	: Model(name), m_priority(nextPriority())
{
}

void AtomicModel::confTransition(const std::vector<n_network::t_msgptr> & message)
{
	this->intTransition();
	this->doExtTransition(message);
}

void AtomicModel::doExtTransition(const std::vector<n_network::t_msgptr>& message)
{
	// Remove all old messages in the input-ports of this model, so the tracer won't find them again
	for (auto& port : m_iPorts)
		port.second->clearReceivedMessages();

	for (auto& m : message) {
		std::string destport = m->getDestinationPort();
		auto it = m_iPorts.begin();
		while (it != m_iPorts.end()) {
			if (n_tools::endswith(destport, it->first))
				break;
			++it;
		}
		// When we find the port, we add the message temporarily to it for the tracer
		if (it != m_iPorts.end())
			it->second->addMessage(m, true);
		else
			LOG_ERROR("Failed to add received message ", m->getPayload(), " to port ", destport);
	}

	// Do the actual external transition
	this->extTransition(message);
}

std::vector<n_network::t_msgptr> AtomicModel::doOutput()
{
	// Remove all old messages in the output-ports of this model, so the tracer won't find them again
	for (auto& port : m_oPorts)
		port.second->clearSentMessages();

	// Do the actual output function
	auto messages = this->output();

	// We return the output back to the core
	return messages;
}

void AtomicModel::setGVT(t_timestamp gvt)
{
	if (!m_keepOldStates) {
		LOG_ERROR("Model has set m_keepOldStates to false, can't call setGVT!");
		return;
	}
	// Model has no memory of past
	if (m_oldStates.empty()) {
		LOG_ERROR("Model has no memory of past (no old states), no GVT happened!");
		return;
	}

	int index = 0;
	int k = -1;

	for (auto& state : m_oldStates) {
		if (state->m_timeLast >= gvt) {
			k = std::max(0, index - 1);
			break;
		}
		index++;
	}

	if (k == -1) {
		// model did not pass gvt yet, keep last state, rest can be forgotten (garbage-collection)
		t_stateptr state = m_oldStates.at(m_oldStates.size() - 1);
		m_oldStates.clear();
		m_oldStates.push_back(state);
	} else if (k == 0) {
		// Do nothing as nothing is to happen
		// m_oldStates has 1 state before the GVT (the first element)
		// the rest of m_oldStates consist of states with a timeLast
		// greater than or equal to our GVT
	} else {
		// Only keep 1 state before GVT, and all states after it
		auto firstState = m_oldStates.begin() + k;
		auto lastState = m_oldStates.end();
		m_oldStates = std::vector<t_stateptr>(firstState, lastState);
	}
}

t_timestamp AtomicModel::revert(t_timestamp time)
{
	if (!m_keepOldStates) {
		LOG_ERROR("Model has set m_keepOldStates to false, can't call setGVT!");
		return t_timestamp::infinity();
	}
	auto r_itStates = m_oldStates.rbegin();
	int index = m_oldStates.size() - 1;

	// We walk over all old states in reverse, and keep track of the index
	assert(!m_oldStates.empty() && "No states to revert to.");
	for (; r_itStates != m_oldStates.rbegin() + (m_oldStates.size() - 1); r_itStates++) {
		if ((*r_itStates)->m_timeLast < time) {
			break;
		}
		index--;
	}

	t_stateptr state = m_oldStates[index];
	this->m_timeLast = state->m_timeLast;
	this->m_timeNext = state->m_timeNext;

	// Pop all obsolete states and set the last old_state as your new state
	this->m_oldStates.resize(index + 1);
	this->m_state = state;

	// We return the m_timeNext of our current state
	LOG_DEBUG("AMODEL:: revert for totime ", time, " returning ", this->m_timeNext);
	return this->m_timeNext;

}

std::size_t AtomicModel::getPriority() const
{
	return m_priority;
}

void AtomicModel::setTime(t_timestamp time)
{
	this->m_timeLast = t_timestamp(time.getTime(), m_priority);
	this->m_timeNext = this->m_timeLast + this->timeAdvance();
	m_state->m_timeLast = this->m_timeLast;
	m_state->m_timeNext = this->m_timeNext;

	// The following is not necessary because they point to the same state
//	t_stateptr laststate = this->m_oldStates.at(this->m_oldStates.size()-1);
//	laststate->m_timeLast = this->m_timeLast;
//	laststate->m_timeNext = this->m_timeNext;
}

t_timestamp AtomicModel::getTimeElapsed() const
{
	return m_elapsed;
}

void AtomicModel::serialize(n_serialisation::t_oarchive& archive)
{
	archive(m_priority, cereal::virtual_base_class<Model>(this));
}

void AtomicModel::serialize(n_serialisation::t_iarchive& archive)
{
	archive(m_priority, cereal::virtual_base_class<Model>(this));
}

void AtomicModel::load_and_construct(n_serialisation::t_iarchive& archive, cereal::construct<AtomicModel>& construct)
{
	std::string name;
	std::size_t priority;
	archive(name, priority);
	construct(name, priority);
}

}

