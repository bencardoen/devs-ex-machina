/*
 * Building.h
 *
 *  Created on: May 22, 2015
 *      Author: pieter
 */

#ifndef SRC_EXAMPLES_TRAFFICSYSTEM_BUILDING_H_
#define SRC_EXAMPLES_TRAFFICSYSTEM_BUILDING_H_

#include "model/atomicmodel.h"
#include "model/state.h"
#include <string>
#include <vector>
#include <random>

namespace n_examples_traffic {

using n_network::t_msgptr;
using n_model::AtomicModel_impl;
using n_model::State;
using n_model::t_stateptr;
using n_model::t_modelptr;
using n_model::t_portptr;
using n_network::t_timestamp;

class BuildingState: public State
{
	friend class Building;

private:
	std::string name;
	std::shared_ptr<std::vector<std::string> > path;
	std::mt19937 randomGenerator;
	t_timestamp currentTime;
	t_timestamp send_query_delay;
	t_timestamp send_car_delay;
	int send_query_id;
	int send_car_id;
	float next_v_pref;
	int sent;


public:
	BuildingState(int IAT_min, int IAT_max, std::shared_ptr<std::vector<std::string> > path, std::string name);
	BuildingState(const BuildingState&);
	std::string toString();

	void setNextVPref(int v_pref_min, int v_pref_max);
	void setSendQueryDelay(int IAT_min, int IAT_max);

	~BuildingState() {}
};

class Building: public AtomicModel_impl
{
protected:


	int IAT_min;
	int IAT_max;
    int v_pref_min;
    int v_pref_max;
    int dv_pos_max;
    int dv_neg_max;

    int send_max;
    int district;

    t_portptr q_sans, q_send, exit, Q_send, car_out;
    t_portptr q_rans, Q_rack, entry;

public:
	Building(bool generator, int district, std::shared_ptr<std::vector<std::string> > path, int IAT_min, int IAT_max,
			 int v_pref_min, int v_pref_max, int dv_pos_max, int dv_neg_max, std::string name);
	~Building() {}

	void extTransition(const std::vector<n_network::t_msgptr> & message) override;
	void intTransition() override;
	t_timestamp timeAdvance() const override;
	std::vector<n_network::t_msgptr> output() const override;

	std::shared_ptr<BuildingState> getBuildingState() const;
};

} // end namespace



#endif /* SRC_EXAMPLES_TRAFFICSYSTEM_BUILDING_H_ */
