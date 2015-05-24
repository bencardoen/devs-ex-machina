/*
 * maindevstone.cpp
 *
 *      Author: Matthijs Van Os - Devs Ex Machina
 */

#include "controllerconfig.h"
#include "coutredirect.h"
#include "devstone.h"

LOG_INIT("devstone.log")

int main(int, char**)
{
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
		t_coupledmodelptr d = n_tools::createObject< n_devstone::DEVStone>(2, 3, false);
		ctrl->addModel(d);

		ctrl->simulate();
	}
}
