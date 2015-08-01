/*
 * queryack.h
 *
 *  Created on: May 31, 2015
 *      Author: pieter
 */

#ifndef QUERYACK_H_
#define QUERYACK_H_

#include <string>
#include "network/timestamp.h"

namespace n_examples_traffic {

using n_network::t_timestamp;

class QueryAck
{
	friend class Building;
	friend class RoadSegment;
	friend class Intersection;

private:
	int ID;
	t_timestamp t_until_dep;

public:
	QueryAck(int ID, t_timestamp t_until_dep);
	QueryAck(const QueryAck&);
	std::string toString();

	friend
	bool operator==(const QueryAck& left, const QueryAck& right);
};
}

#endif /* QUERYACK_H_ */
