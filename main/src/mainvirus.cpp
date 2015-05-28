/*
 * mainvirus.cpp
 *
 *      Author: Matthijs Van Os - Devs Ex Machina
 */

#include "controllerconfig.h"
#include "coutredirect.h"
#include "virus.h"
#include "stringtools.h"

LOG_INIT("virus.log")

/**
 * The executable takes up to 4 arguments (in this order):
 * - The type of simulation:
 * 	+ "classic" 	- Classic DEVS
 * 	+ "pdevs" 	- Optimistic Parallel DEVS
 * 	+ "opdevs"	- Optimistic Parallel DEVS
 * 	+ "cpdevs"	- Conservative Parallel DEVS
 * 	-> If you choose for a parallel simulation, your next argument should specify the amount of cores!
 * - The poolsize (amount of neutral cells)
 * - The connections (amount of connections per cell)
 */
int main(int argc, char** args)
{
	// default values:
	std::string type;
	n_control::Controller::SimType simType = n_control::Controller::CLASSIC;
	int offset = 0;
	int coreAmt = 1;
	std::size_t poolsize = 34;
	std::size_t connections = 6;

	if(argc >= 2) {
		type = args[1];
		if(type == "pdevs" || type == "opdevs" || type == "cpdevs") {
			simType = n_control::Controller::PDEVS;
			if(argc >= 3)
				coreAmt = n_tools::toInt(args[2]);
				++offset;
		}
	}
	if(argc >= offset+3) {
		poolsize = n_tools::toInt(args[offset+2]);
	}
	if(argc >= offset+4) {
		connections = n_tools::toInt(args[offset+3]);
	}

	n_control::ControllerConfig conf;
	conf.name = "Virus";
	conf.simType = simType;
	if (type == "cpdevs")
		conf.pdevsType = n_control::ControllerConfig::CONSERVATIVE;
	conf.coreAmount = coreAmt;
	conf.saveInterval = 5;

	std::ofstream filestream("./virus.txt");
	{
		CoutRedirect myRedirect(filestream);
		auto ctrl = conf.createController();
		t_timestamp endTime(1200, 0);
		ctrl->setTerminationTime(endTime);

		t_coupledmodelptr d = n_tools::createObject< n_virus::Structure>(poolsize, connections);
		ctrl->addModel(d);

		ctrl->simulate();
	}
}
