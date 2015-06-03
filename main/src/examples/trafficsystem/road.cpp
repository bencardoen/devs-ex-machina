/*
 * Road.cpp
 *
 *  Created on: May 22, 2015
 *      Author: pieter
 */

#include "road.h"
#include <sstream>

namespace n_examples_traffic {
Road::Road(int district, std::string name, int segments): CoupledModel(name)
{
	std::vector<std::shared_ptr<RoadSegment> > segment;
	for (int i = 0; i < segments; ++i) {
		std::stringstream ss;
		ss << name << "_" << i;
		std::shared_ptr<RoadSegment> submodel = std::make_shared<RoadSegment>(district, 100, 18, 0.1, ss.str());
		addSubModel(submodel);
		segment.push_back(submodel);
	}

	// in-ports
	q_rans = addInPort("q_rans");
	q_recv = addInPort("q_recv");
	car_in = addInPort("car_in");
	entries = addInPort("entries");
	q_rans_bs = addInPort("q_rans_bs");
	q_recv_bs = addInPort("q_recv_bs");

	// out-ports
	q_sans = addOutPort("q_sans");
	q_send = addOutPort("q_send");
	car_out = addOutPort("car_out");
	exits = addOutPort("exits");
	q_sans_bs = addOutPort("q_sans_bs");

	connectPorts(q_rans, segment.back()->q_rans);
	connectPorts(q_recv, segment.at(0)->q_recv);
	connectPorts(car_in, segment.at(0)->car_in);
	connectPorts(entries, segment.at(0)->entries);
	connectPorts(q_rans_bs, segment.at(0)->q_rans_bs);
	connectPorts(q_recv_bs, segment.at(0)->q_recv_bs);

	connectPorts(segment.at(0)->q_sans, q_sans);
	connectPorts(segment.back()->q_send, q_send);
	connectPorts(segment.back()->car_out, car_out);
	connectPorts(segment.at(0)->exits, exits);
	connectPorts(segment.at(0)->q_sans_bs, q_sans_bs);

	for (int i = 1; i < segments; ++i) {
		connectPorts(segment.at(i)->q_sans, segment.at(i-1)->q_rans);
		connectPorts(segment.at(i-1)->q_send, segment.at(i)->q_recv);
		connectPorts(segment.at(i-1)->car_out, segment.at(i)->car_in);
	}
}

}


