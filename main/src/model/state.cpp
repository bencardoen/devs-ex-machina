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
	archive(m_state);
}

void State::serialize(n_serialization::t_iarchive& archive)
{
	archive(m_state);
}

void State::load_and_construct(n_serialization::t_iarchive& archive, cereal::construct<State>& construct)
{
	std::string state;
	archive(state);
	construct(state);
}

}
