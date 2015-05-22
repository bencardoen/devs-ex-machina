/*
 * Datastructures.h
 *
 *  Created on: May 22, 2015
 *      Author: pieter
 */

#ifndef SRC_EXAMPLES_TRAFFICSYSTEM_DATASTRUCTURES_H_
#define SRC_EXAMPLES_TRAFFICSYSTEM_DATASTRUCTURES_H_

#include "constants.h"

namespace n_examples_traffic {

struct Car
{
	int id;
	int v_pref;
	int dv_pos_max;
	int dv_neg_max;
	int departure_time;
	int distance_travelled;
	int remaining_x;
	int v;

	Car(int id, int v, int v_pref, int dv_pos_max, int dv_neg_max, int departure_time);

	std::string toString();

	friend bool operator==(const Car& lhs, const Car& rhs);
};

struct Query
{
	int id;
	Direction direction;

	Query(int id);

	std::string toString();

	friend bool operator==(const Query& lhs, const Query& rhs);
};

struct QueryAck
{
	int id;
	int t_until_dep;

	QueryAck(int id, int t_until_dep);

	std::string toString();

	friend bool operator==(const QueryAck& lhs, const QueryAck& rhs);
};

} // end namespace


#endif /* SRC_EXAMPLES_TRAFFICSYSTEM_DATASTRUCTURES_H_ */
