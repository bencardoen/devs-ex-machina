/*
 * atomicmodel.cpp
 *
 *  Created on: Mar 17, 2015
 *      Author: tim
 */

#include "atomicmodel.h"

namespace n_model {

AtomicModel::AtomicModel(std::string name, std::size_t priority)
	: Model(name), m_priority(priority)
{

}

void AtomicModel::confTransition(const std::vector<n_network::t_msgptr> & message)
{
	this->intTransition();
	this->extTransition(message);
}

void AtomicModel::setGVT(t_timestamp gvt)
{
	// Model has no memory of past
	if (m_oldStates.empty()) {
		std::cerr << "Model has no memory of past (no old states), no GVT happened!" << std::endl;
		return;
	}

	int index = 0;
	int k = -1;

	for (auto state : m_oldStates) {
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
	} else {
		// Only keep 1 state before GVT, and all states after it
		auto firstState = m_oldStates.begin() + k;
		auto lastState = m_oldStates.end();
		m_oldStates = std::vector<t_stateptr>(firstState, lastState);
	}
}

void AtomicModel::revert(t_timestamp time)
{
	auto r_itStates = m_oldStates.rbegin();
	int index = m_oldStates.size() - 1;

	// We walk over all old states in reverse, and keep track of the index
	for (; r_itStates != m_oldStates.rbegin() + (m_oldStates.size() - 1); r_itStates++) {
		if ((*r_itStates)->m_timeLast < time) {
			break;
		}
		index--;
	}

	t_stateptr state = m_oldStates[index];
	this->m_timeLast = state->m_timeLast;
	this->m_timeNext = state->m_timeNext;

	// Pop all obsolete states
	this->m_oldStates.resize(index);

	this->setState(state);

}

std::size_t AtomicModel::getPriority() const
{
	return m_priority;
}

void AtomicModel::setTime(t_timestamp time)
{
	this->m_timeLast = time;
	this->m_timeNext = time + this->timeAdvance();
	m_state->m_timeLast = this->m_timeLast;
	m_state->m_timeNext = this->m_timeNext;

	// The following is not necessary because they point to the same state
//	t_stateptr laststate = this->m_oldStates.at(this->m_oldStates.size()-1);
//	laststate->m_timeLast = this->m_timeLast;
//	laststate->m_timeNext = this->m_timeNext;
}

}

