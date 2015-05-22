/*
 * state.cpp
 *
 *  Created on: Mar 22, 2015
 *      Author: tim
 */

#include "state.h"
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
	LOG_INFO("STATE: SAVE TimeLast ", m_timeLast, " TimeNext ", m_timeNext, " State ", m_state);
	archive(m_timeLast, m_timeNext, m_state);
}

void State::serialize(n_serialization::t_iarchive& archive)
{
	archive(m_timeLast, m_timeNext, m_state);
	LOG_INFO("STATE: LOAD TimeLast ", m_timeLast, " TimeNext ", m_timeNext, " State ", m_state);
}

void State::load_and_construct(n_serialization::t_iarchive& archive, cereal::construct<State>& construct)
{
	LOG_INFO("STATE: Load and Construct");
	std::string state;
	t_timestamp a, b;
	archive(a, b, state);
	construct(state);
	archive(construct->m_timeLast, construct->m_timeNext);
}

}
