/*
 * query.h
 *
 *  Created on: May 31, 2015
 *      Author: pieter
 */

#ifndef QUERY_H_
#define QUERY_H_

#include <string>

namespace n_examples_traffic {
class Query
{
	friend class RoadSegment;
	friend class Intersection;

private:
	int ID;
	std::string direction;

public:
	Query(int ID);
	Query(const Query&);
	std::string toString() const;

	friend
	bool operator==(const Query& left, const Query& right);

	friend
	std::ostream& operator<<(std::ostream& os, const Query& rhs);
};
}

#endif /* QUERY_H_ */
