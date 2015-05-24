/*
 * mainphold.cpp
 *
 *      Author: Matthijs Van Os - Devs Ex Machina
 */

#include "controllerconfig.h"
#include "coutredirect.h"
#include "phold.h"

LOG_INIT("devstone.log")

int main(int, char**)
{
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
		t_coupledmodelptr d = n_tools::createObject< n_benchmarks_phold::PHOLD>(1, 10, 0, 0.1);
		ctrl->addModel(d);

		ctrl->simulate();
	}
}
