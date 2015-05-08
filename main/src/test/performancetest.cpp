/*
 * performancetest.cpp
 *
 *  Created on: 5 May 2015
 *      Author: matthijs
 */

#include <gtest/gtest.h>
#include <iostream>
#include "coutredirect.h"
#include "controllerconfig.h"
#include "objectfactory.h"
#include "devstone.h"

using namespace n_control;
using namespace n_devstone;
using namespace n_tools;

TEST(Performance, DEVStone)
{
	RecordProperty("description", "Runs DEVStone");

	ControllerConfig conf;
	conf.name = "DEVStone";
	conf.saveInterval = 1;

	std::ofstream filestream("testfiles/performance/devstone.txt");
	{
		CoutRedirect myRedirect(filestream);
		auto ctrl = conf.createController();
		t_timestamp endTime(360, 0);
		ctrl->setTerminationTime(endTime);

		// Create a DEVStone simulation with width 1 and depth 1
		t_coupledmodelptr d = createObject<DEVStone>(1, 1, false); // TODO change width once scheduler problem is solved
		ctrl->addModel(d);

		ctrl->simulate();
	}
}
