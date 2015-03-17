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
AtomicModel::AtomicModel(std::string name)
	: Model(name)
{

}

//AtomicModel::~AtomicModel() {
//
//}

void AtomicModel::confTransition(const n_network::t_msgptr & message)
{
	this->intTransition();
	this->extTransition(message);
}

void AtomicModel::setGVT(t_timestamp gvt)
{

	// Model has no memory of past, TODO: just raise error?
	if (m_oldStates.empty())
		return;

	t_timestamp c(0);

	for (auto state : m_oldStates) {
		if (state->m_timeLast >= gvt)
			;
		// Something of the form: c.first or c.second +1?
	}

	// If never gotten into if_statement of for_loop
	// this->m_oldStates = ...

	// Then some weird shit..

}

void AtomicModel::revert(t_timestamp time)
{
	auto r_itStates = m_oldStates.rbegin();
	t_timestamp stamp(m_oldStates.size());

	for (; r_itStates != m_oldStates.rend(); r_itStates++) {
		if ((*r_itStates)->m_timeLast < time) {
			break;
		}
		// stamp -= 1 ???
	}

	t_stateptr state = m_oldStates[stamp.getTime()]; // ? dit de bedoeling?
	this->m_timeLast = state->m_timeLast;
	this->m_timeNext = state->m_timeNext;

	this->m_state = state; // What do we do here?

	// TODO: do we implement memorization?

	// Pop all obsolete states
	this->m_oldStates.resize((int) stamp.getTime());

	// TODO Check if 1 of reverted states was ever read for the termination condition?
	// What is this?
}

}

