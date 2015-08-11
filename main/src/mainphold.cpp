/*
 * mainphold.cpp
 *
 *      Author: Matthijs Van Os - Devs Ex Machina
 */

#include "control/controllerconfig.h"
#include "tools/coutredirect.h"
#include "performance/benchmarks/phold.h"

LOG_INIT("phold.log")

/**
 * The executable takes up to 6 arguments (in this order):
 * - The type of simulation:
 * 	+ "classic" 	- Classic DEVS
 * 	+ "pdevs" 	- Optimistic Parallel DEVS
 * 	+ "opdevs"	- Optimistic Parallel DEVS
 * 	+ "cpdevs"	- Conservative Parallel DEVS
 * 	-> If you choose for a parallel simulation, your next argument should specify the amount of cores!
 * - the amount of nodes of the PHOLD
 * - the amount of atomic models per node
 * - the amount of iterations
 * - the @b percentage of remotes (example: if you want 42% remotes, enter 42)
 */
int main(int argc, char** args)
{
	// default values:
	int offset = 0; // Parallel sim adds extra argument
	std::string type;
	n_control::SimType simType = n_control::SimType::CLASSIC;
	int coreAmt = 1;
	std::size_t nodes = 1;
	std::size_t apn = 10;
	std::size_t iter = 0;
	float percentageRemotes = 0.1;

	if (argc >= 2) {
		type = args[1];
		if (type == "pdevs" || type == "opdevs" || type == "cpdevs") {
			simType = (type == "cpdevs")? n_control::SimType::CONSERVATIVE : n_control::SimType::OPTIMISTIC;
			if (argc >= 3)
				coreAmt = n_tools::toInt(args[2]);
			++offset;
		}
	}
	if (argc >= offset + 3) {
		nodes = n_tools::toInt(args[offset + 2]);
	}
	if (argc >= offset + 4) {
		apn = n_tools::toInt(args[offset + 3]);
	}
	if (argc >= offset + 5) {
		iter = n_tools::toInt(args[offset + 4]);
	}
	if (argc >= offset + 6) {
		percentageRemotes = n_tools::toInt(args[offset + 5]) / 100;
	}

	n_control::ControllerConfig conf;
	conf.m_name = "DEVStone";
	conf.m_simType = simType;
	conf.m_coreAmount = coreAmt;
	conf.m_saveInterval = 5;
	conf.m_zombieIdleThreshold = 10;

	std::ofstream filestream("./phold.txt");
	{
		n_tools::CoutRedirect myRedirect(filestream);
		auto ctrl = conf.createController();
		t_timestamp endTime(1000, 0);
		ctrl->setTerminationTime(endTime);

		t_coupledmodelptr d = n_tools::createObject<n_benchmarks_phold::PHOLD>(nodes, apn, iter,
		        percentageRemotes);
		ctrl->addModel(d);

		ctrl->simulate();
	}
}
