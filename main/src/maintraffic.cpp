/*
 * maintraffic.cpp
 *
 *  Created on: Jun 11, 2015
 *      Author: pieter
 */

#include "control/controllerconfig.h"
#include "tools/coutredirect.h"
#include "examples/trafficsystem/city.h"

LOG_INIT("traffic.log")

int main(int argc, char** args)
{
	LOG_DEBUG("MAIN: Starting configuration.");
	std::string type;
	n_control::SimType simType = n_control::SimType::CLASSIC;
	int offset = 0;
	int coreAmt = 1;

	if(argc >= 2) {
		type = args[1];
		if(type == "pdevs" || type == "opdevs" || type == "cpdevs") {
			simType = (type == "cpdevs")? n_control::SimType::CONSERVATIVE : n_control::SimType::OPTIMISTIC;
			if(argc >= 3)
				coreAmt = n_tools::toInt(args[2]);
				++offset;
		}
	}

	n_control::ControllerConfig conf;
	conf.m_name = "Traffic";
	conf.m_simType = simType;
	conf.m_coreAmount = coreAmt;
	conf.m_saveInterval = 5;

	std::ofstream filestream("./traffic.txt");
	{
		n_tools::CoutRedirect myRedirect(filestream);
		auto ctrl = conf.createController();
		t_timestamp endTime(1000, 0);
		ctrl->setTerminationTime(endTime);

		LOG_DEBUG("MAIN: Creating the city..");
		n_examples_traffic::City city;
		LOG_DEBUG("MAIN: Creating the city with createObject..");
		t_coupledmodelptr d = n_tools::createObject<n_examples_traffic::City>();
		LOG_DEBUG("MAIN: Created the city.");
		ctrl->addModel(d);
		LOG_DEBUG("MAIN: Added the city model.");

		ctrl->simulate();
	}
}
