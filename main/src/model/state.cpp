/*
 * state.cpp
 *
 *  Created on: Mar 22, 2015
 *      Author: Tim Tuijn
 */

#include "model/state.h"
#include "cereal/types/string.hpp"

namespace n_model {

bool operator==(const State& lhs, const std::string rhs)
{
	return lhs.m_state == rhs;
}
bool operator==(const std::string lhs, const State& rhs)
{
	return rhs == lhs;
}

void State::serialize(n_serialization::t_oarchive& archive)
{
	LOG_INFO("SERIALIZATION: Saving State '", m_state, "' with timeNext = ", m_timeNext, " and timeLast = ", m_timeLast);
	archive(m_state, m_timeLast, m_timeNext);
}

void State::serialize(n_serialization::t_iarchive& archive)
{
	archive(m_state, m_timeLast, m_timeNext);
	LOG_INFO("SERIALIZATION: Loaded State '", m_state, "' with timeNext = ", m_timeNext, " and timeLast = ", m_timeLast);
}

void State::load_and_construct(n_serialization::t_iarchive& archive, cereal::construct<State>& construct)
{
	LOG_DEBUG("STATE: Load and Construct");
	std::string state;
	archive(state);
	construct(state);
	archive(construct->m_timeLast, construct->m_timeNext);
	LOG_INFO("SERIALIZATION: Loaded State '", construct->m_state, "' with timeNext = ", construct->m_timeNext, " and timeLast = ", construct->m_timeLast);
}

}
