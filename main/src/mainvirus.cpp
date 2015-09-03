/*
 * mainvirus.cpp
 *
 *      Author: Matthijs Van Os - Devs Ex Machina
 */

#ifndef VIRUSTRACER
#define VIRUSTRACER;	//added this line so that the poor Eclipse CDT indexer knows that this option should be set.
static_assert(false, "Virus simulation needs the VIRUSTRACER macro set in the compilation options.\n"
			"please add the following setting to the build settings of this build: -DVIRUSTRACER=1");
#endif

#include "control/controllerconfig.h"
#include "tools/coutredirect.h"
#include "examples/virus/virus.h"
#include "tools/stringtools.h"

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
	n_control::SimType simType = n_control::SimType::CLASSIC;
	int offset = 0;
	int coreAmt = 1;
	std::size_t poolsize = 34;
	std::size_t connections = 6;

	if(argc >= 2) {
		type = args[1];
		if(type == "pdevs" || type == "opdevs" || type == "cpdevs") {
			simType = (type == "cpdevs")? n_control::SimType::CONSERVATIVE : n_control::SimType::OPTIMISTIC;
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
	conf.m_name = "Virus";
	conf.m_simType = simType;
	conf.m_coreAmount = coreAmt;
	conf.m_saveInterval = 5;
	conf.m_tracerset = n_tools::createObject<n_tracers::t_tracerset>();
	conf.m_tracerset->getByID<0>().initialize("./virus.txt");
	conf.m_tracerset->getByID<1>().initialize("./virus", ".dot");
//	conf.tracerset->getByID<1>().initialize();
	//create the controller
	auto ctrl = conf.createController();
	t_timestamp endTime(1200, 0);
	ctrl->setTerminationTime(endTime);

	t_coupledmodelptr d = n_tools::createObject< n_virus::Structure>(poolsize, connections);
	ctrl->addModel(d);

	//fire the simulation!
	ctrl->simulate();
}
