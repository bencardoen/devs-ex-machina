/*
 * query.cpp
 *
 *  Created on: Jun 1, 2015
 *      Author: pieter
 */

#include "examples/trafficsystem/query.h"
#include <sstream>

namespace n_examples_traffic {

Query::Query(int ID): ID(ID)
{
	direction = "";
}

Query::Query(const Query& query): Query(query.ID)
{
	direction = query.direction;
}

std::string Query::toString() const
{
	std::stringstream ss;
	ss << "Query: ID = " << ID;
	return ss.str();
}

bool operator==(const Query& left, const Query& right)
{
	return (left.ID == right.ID and left.direction == right.direction);
}

std::ostream& operator<<(std::ostream& os, const Query& rhs)
{
	os << rhs.toString();
	return os;
}

}


