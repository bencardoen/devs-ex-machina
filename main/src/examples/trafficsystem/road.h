/*
 * Road.h
 *
 *  Created on: May 22, 2015
 *      Author: pieter
 */

#ifndef SRC_EXAMPLES_TRAFFICSYSTEM_ROAD_H_
#define SRC_EXAMPLES_TRAFFICSYSTEM_ROAD_H_

#include "coupledmodel.h"
#include "state.h"
#include "roadsegment.h"
#include <vector>

namespace n_examples_traffic {

using n_model::CoupledModel;
using n_network::t_msgptr;
using n_model::t_portptr;

class Road: public CoupledModel
{
private:
	t_portptr q_rans, q_recv, car_in, entries, q_rans_bs, q_recv_bs;
	t_portptr q_sans, q_send, car_out, exits, q_sans_bs;

public:
	Road(int district, std::string name = "Road", int segments = 5);
	~Road() {}
};

}



#endif /* SRC_EXAMPLES_TRAFFICSYSTEM_ROAD_H_ */
