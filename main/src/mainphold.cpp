/*
 * mainphold.cpp
 *
 *      Author: Matthijs Van Os - Devs Ex Machina
 */

#include "controllerconfig.h"
#include "coutredirect.h"
#include "phold.h"

LOG_INIT("phold.log")


/**
 * The executable takes up to 4 arguments (in this order):
 * - the amount of nodes of the PHOLD
 * - the amount of atomic models per node
 * - the amount of iterations
 * - the @b percentage of remotes (example: if you want 42% remotes, enter 42)
 */
int main(int argc, char** args)
{
	// default values:
	std::size_t nodes = 1;
	std::size_t apn = 10;
	std::size_t iter = 0;
	float percentageRemotes = 0.1;

	if(argc >= 2) {
		nodes = n_tools::toInt(args[1]);
	}
	if(argc >= 3) {
		apn = n_tools::toInt(args[2]);
	}
	if(argc >= 4) {
		iter = n_tools::toInt(args[3]);
	}
	if(argc >= 5) {
		percentageRemotes = n_tools::toInt(args[4]) / 100;
	}


	n_control::ControllerConfig conf;
	conf.name = "DEVStone";
	conf.saveInterval = 1;

	std::ofstream filestream("./phold.txt");
	{
		CoutRedirect myRedirect(filestream);
		auto ctrl = conf.createController();
		t_timestamp endTime(1000, 0);
		ctrl->setTerminationTime(endTime);

		// Create a DEVStone simulation with width 2 and depth 3
		t_coupledmodelptr d = n_tools::createObject< n_benchmarks_phold::PHOLD>(nodes, apn, iter, percentageRemotes);
		ctrl->addModel(d);

		ctrl->simulate();
	}
}
