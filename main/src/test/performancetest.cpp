/*
 * performancetest.cpp
 *
 *  Created on: 5 May 2015
 *      Author: matthijs
 */

#include <gtest/gtest.h>
#include <iostream>
#include "test/compare.h"
#include "tools/coutredirect.h"
#include "control/controllerconfig.h"
#include "tools/objectfactory.h"
#include "performance/devstone/devstone.h"
#include "performance/benchmarks/phold.h"
#include "examples/trafficsystem/city_small.h"

using namespace n_control;
using namespace n_devstone;
using namespace n_benchmarks_phold;
using namespace n_tools;

TEST(Performance, DEVStone)
{
	RecordProperty("description", "Runs DEVStone");

	ControllerConfig conf;
	conf.name = "DEVStone";
	conf.saveInterval = 1;

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

TEST(Performance, TrafficSystem)
{
	ControllerConfig conf;
	conf.name = "TrafficSystem";

	std::ofstream filestream(TESTFOLDER "performance/traffic.txt");
	{
		CoutRedirect myRedirect(filestream);
		auto ctrl = conf.createController();
		t_timestamp endTime(1000, 0);
		n_examples_traffic::City city;
		ctrl->setTerminationTime(endTime);

		t_coupledmodelptr d = n_tools::createObject<n_examples_traffic::City>();
		ctrl->addModel(d);

		ctrl->simulate();
	}
}

TEST(Performance, PHOLD)
{
	RecordProperty("description", "Runs the PHOLD benchmark");

	ControllerConfig conf;
	conf.name = "PHOLDBench";
	conf.saveInterval = 1;

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
