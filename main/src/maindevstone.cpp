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
 * The executable takes up to 2 arguments (in this order):
 * - The width (amount of processors per coupled model)
 * - The depth (amount of linked coupled models)
 */
int main(int argc, char** args)
{
	// default values:
	std::size_t width = 2;
	std::size_t depth = 3;

	if(argc >= 2) {
		width = n_tools::toInt(args[1]);
	}
	if(argc >= 3) {
		depth = n_tools::toInt(args[2]);
	}

	n_control::ControllerConfig conf;
	conf.name = "DEVStone";
	conf.saveInterval = 1;

	std::ofstream filestream("./devstone.txt");
	{
		CoutRedirect myRedirect(filestream);
		auto ctrl = conf.createController();
		t_timestamp endTime(1000, 0);
		ctrl->setTerminationTime(endTime);

		// Create a DEVStone simulation with width 2 and depth 3
		t_coupledmodelptr d = n_tools::createObject< n_devstone::DEVStone>(width, depth, false);
		ctrl->addModel(d);

		ctrl->simulate();
	}
}
