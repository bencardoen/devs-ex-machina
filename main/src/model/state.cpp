/*
 * state.cpp
 *
 *  Created on: Mar 22, 2015
 *      Author: tim
 */

#include "state.h"

namespace n_model {

bool operator==(const State& lhs, const std::string rhs)
{
	return lhs.m_state == rhs;
}
bool operator==(const std::string lhs, const State& rhs)
{
	return rhs == lhs;
}

}
