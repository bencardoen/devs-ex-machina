/*
 * car.h
 *
 *  Created on: May 31, 2015
 *      Author: pieter
 */

#ifndef CAR_H_
#define CAR_H_

#include <vector>
#include <string>
#include "network/timestamp.h"

namespace n_examples_traffic {

using n_network::t_timestamp;

class Car
{
	friend class Building;
	friend class RoadSegment;
	friend class Intersection;

private:
	int ID;
	float v;
	float v_pref;
	int dv_pos_max;
	int dv_neg_max;
	t_timestamp departure_time;
	int distance_travelled;
	int remaining_x;
	std::vector<std::string> path;

public:
	Car(int ID, float v, float v_pref, int dv_pos_max, int dv_neg_max, t_timestamp departure_time);
	Car(const Car&);
	std::string toString() const;

	int getID() const;

	friend
	bool operator==(const Car& left, const Car& right);

	friend
	std::ostream& operator<<(std::ostream& os, const Car& rhs);
};

}

#endif /* CAR_H_ */
