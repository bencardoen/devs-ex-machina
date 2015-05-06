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
	conf.simType = Controller::PDEVS;
	conf.coreAmount = 2;
	conf.saveInterval = 30;

//	std::ofstream filestream("testfiles/performance/devstone.txt");
//	{
//		CoutRedirect myRedirect(filestream);
		auto ctrl = conf.createController();
		t_timestamp endTime(2000, 0);
		ctrl->setTerminationTime(endTime);

		t_coupledmodelptr d = createObject<DEVStone>(1, 1, false);
		ctrl->addModel(d);

		ctrl->simulate();
//	}
}
