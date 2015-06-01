/*
 * queryack.cpp
 *
 *  Created on: Jun 1, 2015
 *      Author: pieter
 */

#include "queryack.h"
#include <sstream>

namespace n_examples_traffic {
QueryAck::QueryAck(int ID, t_timestamp t_until_dep): ID(ID), t_until_dep(t_until_dep)
{
}

QueryAck::QueryAck(const QueryAck& queryAck): QueryAck(queryAck.ID, queryAck.t_until_dep)
{
}

std::string QueryAck::toString()
{
	std::stringstream ss;
	ss << "Query Ack: ID = " << ID;
	ss << ", t_until_dep = " << t_until_dep;
	return ss.str();
}

bool operator==(const QueryAck& left, const QueryAck& right)
{
	return (left.ID == right.ID and left.t_until_dep == right.t_until_dep);
}

}


