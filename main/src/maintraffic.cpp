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
	n_control::Controller::SimType simType = n_control::Controller::CLASSIC;
	int offset = 0;
	int coreAmt = 1;

	if(argc >= 2) {
		type = args[1];
		if(type == "pdevs" || type == "opdevs" || type == "cpdevs") {
			simType = n_control::Controller::PDEVS;
			if(argc >= 3)
				coreAmt = n_tools::toInt(args[2]);
				++offset;
		}
	}

	n_control::ControllerConfig conf;
	conf.name = "Traffic";
	conf.simType = simType;
	if (type == "cpdevs")
		conf.pdevsType = n_control::ControllerConfig::CONSERVATIVE;
	conf.coreAmount = coreAmt;
	conf.saveInterval = 5;

	std::ofstream filestream("./traffic.txt");
	{
		CoutRedirect myRedirect(filestream);
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
