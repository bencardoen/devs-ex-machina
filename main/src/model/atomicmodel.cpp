/*
 * atomicmodel.cpp
 *
 *  Created on: Mar 17, 2015
 *      Author: Tim Stijn
 */
#include "tools/globallog.h"

#include "model/atomicmodel.h"
#include "cereal/types/base_class.hpp"

namespace n_model {

AtomicModel_impl::AtomicModel_impl(std::string name, std::size_t)
	: Model(name), m_corenumber(-1), m_keepOldStates(true), m_state(nullptr), m_priority(nextPriority()),m_transition_type_next(NONE)
{
        LOG_DEBUG("\tAMODEL ctor :: name=", name, " m_prior= ", m_priority , " corenr=", m_corenumber);
}

AtomicModel_impl::AtomicModel_impl(std::string name, int corenumber, std::size_t priority)
	: Model(name), m_corenumber(corenumber), m_keepOldStates(true), m_state(nullptr), m_priority(nextPriority()),m_transition_type_next(NONE)
{
        if(m_priority == std::numeric_limits<std::size_t>::max())
                m_priority = nextPriority();
        else
                m_priority = priority;
        LOG_DEBUG("\tAMODEL ctor :: name=", name, " m_prior= ", m_priority , " corenr=", m_corenumber);
}

void AtomicModel_impl::intTransition()
{
	LOG_ERROR("ATOMICMODEL: Not implemented: 'void n_model::AtomicModel::intTransition()'");
	assert(false);
}

void AtomicModel_impl::confTransition(const std::vector<n_network::t_msgptr> & message)
{
	this->intTransition();
	this->extTransition(message);
}

void AtomicModel_impl::extTransition(const std::vector<n_network::t_msgptr>&)
{
	LOG_ERROR(
		"ATOMICMODEL: Not implemented: 'void n_model::AtomicModel::extTransition(const std::vector<n_network::t_msgptr> & message)'");
	assert(false);
}

void AtomicModel_impl::doExtTransition(const std::vector<n_network::t_msgptr>& message)
{
	// Remove all old messages in the input-ports of this model, so the tracer won't find them again
	for (t_portptr& port : m_iPorts)
		port->clearReceivedMessages();

	deliverMessages(message);

	//copy the current state, if necessary
	copyState();

	// Do the actual external transition
	this->extTransition(message);
}

void AtomicModel_impl::doIntTransition()
{
	for (auto& port : m_iPorts)
		port->clearReceivedMessages();

	//copy the current state, if necessary
	copyState();

	intTransition();
}


void AtomicModel_impl::doConfTransition(const std::vector<n_network::t_msgptr>& message)
{
	// Remove all old messages in the input-ports of this model, so the tracer won't find them again
	for (auto& port : m_iPorts)
		port->clearReceivedMessages();

	deliverMessages(message);

	//copy the current state, if necessary
	copyState();

	this->confTransition(message);
}

void AtomicModel_impl::doOutput(std::vector<n_network::t_msgptr>& msgs)
{
	// Remove all old messages in the output-ports of this model, so the tracer won't find them again
	LOG_DEBUG("Atomic Model ::  ", this->getName(), " clearing sent messages.");
#ifndef NO_TRACER
	for (auto& port : m_oPorts){
		port->clearSentMessages();
	}
#endif
	LOG_DEBUG("Atomic Model ::  ", this->getName(), " calling output() function. ");
	// Do the actual output function
	this->output(msgs);
	LOG_DEBUG("Atomic Model ::  ", this->getName(), " output resulted in ", msgs.size(), " messages ");
}

void AtomicModel_impl::setGVT(t_timestamp gvt)
{
	assert(m_keepOldStates && "AtomicModel_impl::setGVT called while old states are not remembered.");
	if (!m_keepOldStates) {
		LOG_ERROR("Model has set m_keepOldStates to false, can't call setGVT!");
		return;
	}
	assert(!m_oldStates.empty() && "AtomicModel_impl::setGVT no memory!");
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
		t_stateptr state = m_oldStates.back();
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

t_timestamp AtomicModel_impl::revert(t_timestamp time)
{
	if (!m_keepOldStates) {
		LOG_ERROR("Model has set m_keepOldStates to false, can't call revert!");
		return t_timestamp::infinity();
	}
	auto r_itStates = m_oldStates.rbegin();
	int index = m_oldStates.size() - 1;

	if(m_oldStates.empty()){
		LOG_ERROR("Model has no old states to revert to!");
		throw std::logic_error("Model  has no states to revert to.");
	}

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

	// Pop all obsolete states and set the last old_state as your new state
	this->m_oldStates.resize(index + 1);
	this->m_state = state;

	// We return the m_timeNext of our current state
	LOG_DEBUG("AMODEL:: revert for totime ", time, " returning ", this->m_timeNext);
	return this->m_timeNext;

}

std::size_t AtomicModel_impl::getPriority() const
{
	return m_priority;
}

void AtomicModel_impl::setTime(t_timestamp time)
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

t_timestamp AtomicModel_impl::getTimeElapsed() const
{
	return m_elapsed;
}

void AtomicModel_impl::addInfluencees(std::vector<std::size_t>& influences) const
{
	for (auto& port: this->m_iPorts)
		port->addInfluencees(influences);
}

void AtomicModel_impl::setTimeElapsed(t_timestamp elapsed)
{
	m_elapsed = elapsed;
}

void AtomicModel_impl::serialize(n_serialization::t_oarchive& archive)
{
	LOG_INFO("SERIALIZATION: Saving Atomic Model '", getName(), "' with timeNext = ", m_timeNext);
	archive(cereal::virtual_base_class<Model>( this ), m_priority, m_corenumber, m_elapsed, m_lastRead);
}

void AtomicModel_impl::serialize(n_serialization::t_iarchive& archive)
{
	archive(cereal::virtual_base_class<Model>( this ), m_priority, m_corenumber, m_elapsed, m_lastRead);
	LOG_INFO("SERIALIZATION: Loaded Atomic Model '", getName(), "' with timeNext = ", m_timeNext);
}

void AtomicModel_impl::deliverMessages(const std::vector<n_network::t_msgptr>& message)
{
#ifdef SAFETY_CHECKS
	for(const n_network::t_msgptr& msg: message)
		m_iPorts.at(msg->getDstPort())->addMessage(msg);
#else /* SAFETY_CHECKS */
	for(const n_network::t_msgptr& msg: message)
		m_iPorts[msg->getDstPort()]->addMessage(msg);
#endif /* SAFETY_CHECKS */
}

void AtomicModel_impl::load_and_construct(n_serialization::t_iarchive& archive, cereal::construct<AtomicModel_impl>& construct)
{
	LOG_DEBUG("ATOMICMODEL: Load and Construct");

	construct("temp");
	construct->serialize(archive);
}

int AtomicModel_impl::getCorenumber() const
{
	return m_corenumber;
}

void AtomicModel_impl::setCorenumber(int corenumber)
{
	m_corenumber = corenumber;
}

t_timestamp AtomicModel_impl::getTimeNext() const
{
	return m_timeNext;
}

t_timestamp AtomicModel_impl::getTimeLast() const
{
	return m_timeLast;
}

bool AtomicModel_impl::getKeepOldStates() const
{
	return m_keepOldStates;
}

void AtomicModel_impl::setKeepOldStates(bool b)
{
	m_keepOldStates = b;
}

const t_stateptr& AtomicModel_impl::getState() const
{
	return m_state;
}

t_stateptr& AtomicModel_impl::getState()
{
	return m_state;
}

void AtomicModel_impl::initState(const t_stateptr& stateptr)
{
	assert(m_state == nullptr && "AtomicModel_impl::initState called while a valid state is already present.");
	m_state = stateptr;
	if(m_keepOldStates)
		m_oldStates.push_back(stateptr);
}

//void AtomicModel_impl::setState(const t_stateptr& newState)
//{
//	if (newState == nullptr)
//		return;
//	m_state = newState;
//	if (m_keepOldStates)
//		m_oldStates.push_back(m_state);
//	else {
//		if (m_oldStates.size() != 0)
//			m_oldStates.at(0) = m_state;
//		else
//			m_oldStates.push_back(m_state);
//	}
//}

void AtomicModel_impl::copyState()
{
	if(!m_keepOldStates)
		return;
	t_stateptr copy = m_state->copyState();
	assert(copy != nullptr && "AtomicModel_impl::copyState received nullptr as copy.");

	m_oldStates.push_back(copy);
	m_state = copy;
}

}
