/*
 * Residence.cpp
 *
 *  Created on: May 22, 2015
 *      Author: pieter
 */

#include "residence.h"

namespace n_examples_traffic {

Residence::Residence(std::vector<std::string> path, int district, std::string name, int IAT_min, int IAT_max,
		  int v_pref_min, int v_pref_max, int dv_pos_max, int dv_neg_max):
		Building(true, district, path, IAT_min, IAT_max, v_pref_min, v_pref_max,
				 dv_pos_max, dv_neg_max, name)
{

}

}


