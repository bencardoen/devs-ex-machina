/*
 * maindevstone.cpp
 *
 *      Author: Matthijs Van Os - Devs Ex Machina
 */

#include "controllerconfig.h"
#include "coutredirect.h"
#include "devstone.h"
#include "stringtools.h"

LOG_INIT("devstone.log")

/**
 * The executable takes up to 4 arguments (in this order):
 * - The type of simulation:
 * 	+ "classic" 	- Classic DEVS
 * 	+ "pdevs" 	- Optimistic Parallel DEVS
 * 	+ "opdevs"	- Optimistic Parallel DEVS
 * 	+ "cpdevs"	- Conservative Parallel DEVS
 * 	-> If you choose for a parallel simulation, your next argument should specify the amount of cores!
 * - The width (amount of processors per coupled model)
 * - The depth (amount of linked coupled models)
 */
int main(int argc, char** args)
{
	// default values:
	std::string type;
	n_control::Controller::SimType simType = n_control::Controller::CLASSIC;
	int widthpos = 2;
	int depthpos = 3;
	int coreAmt = 1;
	std::size_t width = 2;
	std::size_t depth = 3;

	if(argc >= 2) {
		type = args[1];
		if(type == "pdevs" || type == "opdevs" || type == "cpdevs") {
			simType = n_control::Controller::PDEVS;
			if(argc >= 3)
				coreAmt = n_tools::toInt(args[2]);
				++depthpos;
				++widthpos;
		}
	}
	if(argc >= widthpos+1) {
		width = n_tools::toInt(args[widthpos]);
	}
	if(argc >= depthpos+1) {
		depth = n_tools::toInt(args[depthpos]);
	}

	n_control::ControllerConfig conf;
	conf.name = "DEVStone";
	conf.simType = simType;
	if (type == "cpdevs")
		conf.pdevsType = n_control::ControllerConfig::CONSERVATIVE;
	conf.coreAmount = coreAmt;
	conf.saveInterval = 5;

	std::ofstream filestream("./devstone.txt");
	{
		CoutRedirect myRedirect(filestream);
		auto ctrl = conf.createController();
		t_timestamp endTime(1000, 0);
		ctrl->setTerminationTime(endTime);

		t_coupledmodelptr d = n_tools::createObject< n_devstone::DEVStone>(width, depth, false);
		ctrl->addModel(d);

		ctrl->simulate();
	}
}
