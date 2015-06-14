/*
 * Residence.h
 *
 *  Created on: May 22, 2015
 *      Author: pieter
 */

#ifndef SRC_EXAMPLES_TRAFFICSYSTEM_RESIDENCE_H_
#define SRC_EXAMPLES_TRAFFICSYSTEM_RESIDENCE_H_

#include "building.h"

namespace n_examples_traffic {

class Residence: public Building
{
	friend class City;

public:
    Residence(std::shared_ptr<std::vector<std::string> > path, int district, std::string name, int IAT_min, int IAT_max,
    		  int v_pref_min, int v_pref_max, int dv_pos_max, int dv_neg_max);
	~Residence() {}
};

} // end namespace



#endif /* SRC_EXAMPLES_TRAFFICSYSTEM_RESIDENCE_H_ */
