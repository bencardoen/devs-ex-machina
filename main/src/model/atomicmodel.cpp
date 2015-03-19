/*
 * atomicmodel.cpp
 *
 *  Created on: Mar 17, 2015
 *      Author: tim
 */

#include "atomicmodel.h"

namespace n_model {

/*
 * Constructor for AtomicModel
 *
 * @param name The name of the model
 */
AtomicModel::AtomicModel(std::string name, std::size_t priority)
	: Model(name), m_priority(priority)
{

}

void AtomicModel::confTransition(const n_network::t_msgptr & message)
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
			k = std::max(0,index-1);
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
	for (; r_itStates != m_oldStates.rbegin() + (m_oldStates.size()-1) ; r_itStates++) {
		if ((*r_itStates)->m_timeLast < time) {
			break;
		}
		index--;
	}

	t_stateptr state = m_oldStates[index];
	this->m_timeLast = state->m_timeLast;
	this->m_timeNext = state->m_timeNext;

	this->m_state = state;

	// Pop all obsolete states
	this->m_oldStates.resize(index+1);
}

std::size_t AtomicModel::getPriority() const {
	return m_priority;
}

}

