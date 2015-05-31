/*
 * performancetest.cpp
 *
 *  Created on: 5 May 2015
 *      Author: matthijs
 */

#include <gtest/gtest.h>
#include <iostream>
#include "compare.h"
#include "coutredirect.h"
#include "controllerconfig.h"
#include "objectfactory.h"
#include "devstone.h"
#include "phold.h"

using namespace n_control;
using namespace n_devstone;
using namespace n_benchmarks_phold;
using namespace n_tools;

TEST(Performance, DEVStone)
{
	RecordProperty("description", "Runs DEVStone");

	ControllerConfig conf;
	conf.name = "DEVStone";
	conf.simType = Controller::PDEVS;
	conf.coreAmount = 2;
	conf.saveInterval = 1;
	conf.zombieIdleThreshold = 10;

	std::ofstream filestream(TESTFOLDER "performance/devstone.txt");
	{
		CoutRedirect myRedirect(filestream);
		auto ctrl = conf.createController();
		t_timestamp endTime(360, 0);
		ctrl->setTerminationTime(endTime);

		// Create a DEVStone simulation with width 2 and depth 3
		t_coupledmodelptr d = createObject<DEVStone>(2, 3, false);
		ctrl->addModel(d);

		ctrl->simulate();
	}
}

TEST(Performance, PHOLD)
{
	RecordProperty("description", "Runs the PHOLD benchmark");

	ControllerConfig conf;
	conf.name = "PHOLDBench";
	conf.simType = Controller::PDEVS;
	conf.coreAmount = 2;
	conf.saveInterval = 1;
	conf.zombieIdleThreshold = 10;

	std::ofstream filestream(TESTFOLDER "/performance/phold.txt");
	{
		CoutRedirect myRedirect(filestream);
		auto ctrl = conf.createController();
		t_timestamp endTime(360, 0);
		ctrl->setTerminationTime(endTime);

		t_coupledmodelptr d = createObject<PHOLD>(1, 10, 128, 0.1);
		ctrl->addModel(d);

		ctrl->simulate();
	}
}
