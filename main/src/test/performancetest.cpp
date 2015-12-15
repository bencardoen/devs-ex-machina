/*
 * This file is part of the DEVS Ex Machina project.
 * Copyright 2014 - 2015 University of Antwerp
 * https://www.uantwerpen.be/en/
 * Licensed under the EUPL V.1.1
 * A full copy of the license is in COPYING.txt, or can be found at
 * https://joinup.ec.europa.eu/community/eupl/og_page/eupl
 *      Author: Ben Cardoen, Stijn Manhaeve, Tim Tuijn, Matthijs Van Os, Pieter Lauwers
 */

#include <gtest/gtest.h>
#include <performance/phold/phold.h>
#include <iostream>
#include "test/compare.h"
#include "tools/coutredirect.h"
#include "control/controllerconfig.h"
#include "tools/objectfactory.h"
#include "performance/devstone/devstone.h"

using namespace n_control;
using namespace n_devstone;
using namespace n_benchmarks_phold;
using namespace n_tools;

TEST(Performance, DEVStone)
{
	RecordProperty("description", "Runs DEVStone");

	ControllerConfig conf;
	conf.m_name = "DEVStone";
	conf.m_saveInterval = 1;

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
	conf.m_name = "PHOLDBench";
	conf.m_saveInterval = 1;

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
