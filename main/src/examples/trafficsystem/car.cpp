/*
 * car.cpp
 *
 *  Created on: May 31, 2015
 *      Author: pieter
 */

#include "car.h"
#include <sstream>

namespace n_examples_traffic {

Car::Car(int ID, float v, float v_pref, int dv_pos_max, int dv_neg_max, t_timestamp departure_time):
		ID(ID), v(v), v_pref(v_pref), dv_pos_max(dv_pos_max), dv_neg_max(dv_neg_max), departure_time(departure_time)
{
	distance_travelled = 0;
	remaining_x = 0;
}

bool operator==(const Car& left, const Car& right)
{
	return (left.ID == right.ID and
			left.v_pref == right.v_pref and
            left.dv_pos_max == right.dv_pos_max and
            left.dv_neg_max == right.dv_neg_max and
            left.departure_time == right.departure_time and
            left.distance_travelled == right.distance_travelled and
            left.remaining_x == right.remaining_x and
            left.v == right.v);
}

Car::Car(const Car& car): Car(car.ID, car.v, car.v_pref, car.dv_pos_max,
							  car.dv_neg_max, car.departure_time)
{
	distance_travelled = car.distance_travelled;
	remaining_x = car.remaining_x;
	path = car.path;
}

std::string Car::toString() const
{
	std::stringstream ss;
	ss << "Car: ID = " << ID;
	ss << ", v_pref = " << v_pref;
	ss << ", dv_pos_max = " << dv_pos_max;
	ss << ", dv_neg_max = " << dv_neg_max;
	ss << ", departure_time = " << departure_time;
	ss << ", distance_travelled = " << distance_travelled;
	ss << ", v = " << v;
	//ss << ", path = " << path;; TODO: check if necessary

	return ss.str();
}

std::ostream& operator<<(std::ostream& os, const Car& rhs)
{
	os << rhs.toString();
	return os;
}

int Car::getID() const
{
	return ID;
}

}
