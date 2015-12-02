/*
 * benchmarktest.cpp
 *
 *  Created on: Oct 30, 2015
 *      Author: Devs Ex Machina
 */


#include <gtest/gtest.h>

#include "control/controllerconfig.h"
#include "tools/coutredirect.h"
#include "performance/devstone/devstone.h"
#include "performance/phold/phold.h"
#include "performance/highInterconnect/hinterconnect.h"
#include "tools/stringtools.h"
#include "control/allocator.h"
#include "test/compare.h"


using namespace n_control;
using namespace n_model;
using namespace n_tools;
using namespace n_network;

#define SUBTESTFOLDER TESTFOLDER "benchmark/"

constexpr t_timestamp::t_time eTime = 1000;


TEST(Benchmark, devstone_single)
{
    LOG_MOVE("logs/bmarkDevstoneSingle.log", false);
	n_control::ControllerConfig conf;
	conf.m_name = "DEVStone";
	conf.m_simType = n_control::SimType::CLASSIC;
	conf.m_coreAmount = 4;
	conf.m_saveInterval = 250;
	conf.m_allocator = n_tools::createObject<n_devstone::DevstoneAlloc>();
	std::size_t width = 5;
	std::size_t depth = 5;
	bool randTa = false;

	auto ctrl = conf.createController();
	t_timestamp endTime(eTime, 0);
	ctrl->setTerminationTime(endTime);

	t_coupledmodelptr d = n_tools::createObject< n_devstone::DEVStone>(width, depth, randTa);
	ctrl->addModel(d);
	std::ofstream filestream(SUBTESTFOLDER "devstoneSingle.txt");
	{
		CoutRedirect myRedirect(filestream);
		ctrl->simulate();
	};

	EXPECT_EQ(n_misc::filecmp(SUBTESTFOLDER "devstoneSingle.txt", SUBTESTFOLDER "devstoneSingle.corr"), 0);
    LOG_MOVE("out.txt", true);
}

TEST(Benchmark, devstone_opt)
{
    LOG_MOVE("logs/bmarkDevstoneOpt.log", false);
	n_control::ControllerConfig conf;
	conf.m_name = "DEVStone";
	conf.m_simType = n_control::SimType::OPTIMISTIC;
	conf.m_coreAmount = 4;
	conf.m_saveInterval = 250;
	conf.m_allocator = n_tools::createObject<n_devstone::DevstoneAlloc>();
	std::size_t width = 5;
	std::size_t depth = 5;
	bool randTa = false;

	auto ctrl = conf.createController();
	t_timestamp endTime(eTime, 0);
	ctrl->setTerminationTime(endTime);

	t_coupledmodelptr d = n_tools::createObject< n_devstone::DEVStone>(width, depth, randTa);
	ctrl->addModel(d);
	std::ofstream filestream(SUBTESTFOLDER "devstoneOptimistic.txt");
	{
		CoutRedirect myRedirect(filestream);
		ctrl->simulate();
	};

	EXPECT_EQ(n_misc::filecmp(SUBTESTFOLDER "devstoneOptimistic.txt", SUBTESTFOLDER "devstoneSingle.corr"), 0);
    LOG_MOVE("out.txt", true);
}

TEST(Benchmark, devstone_cons)
{
    LOG_MOVE("logs/bmarkDevstoneCons.log", false);
	n_control::ControllerConfig conf;
	conf.m_name = "DEVStone";
	conf.m_simType = n_control::SimType::CONSERVATIVE;
	conf.m_coreAmount = 4;
	conf.m_saveInterval = 250;
	conf.m_allocator = n_tools::createObject<n_devstone::DevstoneAlloc>();
	std::size_t width = 5;
	std::size_t depth = 5;
	bool randTa = false;

	auto ctrl = conf.createController();
	t_timestamp endTime(eTime, 0);
	ctrl->setTerminationTime(endTime);

	t_coupledmodelptr d = n_tools::createObject< n_devstone::DEVStone>(width, depth, randTa);
	ctrl->addModel(d);
	std::ofstream filestream(SUBTESTFOLDER "devstoneConservative.txt");
	{
		CoutRedirect myRedirect(filestream);
		ctrl->simulate();
	};

	EXPECT_EQ(n_misc::filecmp(SUBTESTFOLDER "devstoneConservative.txt", SUBTESTFOLDER "devstoneSingle.corr"), 0);
    LOG_MOVE("out.txt", true);
}

TEST(Benchmark, devstone_single_r)
{
    LOG_MOVE("logs/bmarkDevstoneSingleR.log", false);
	n_control::ControllerConfig conf;
	conf.m_name = "DEVStone";
	conf.m_simType = n_control::SimType::CLASSIC;
	conf.m_coreAmount = 4;
	conf.m_saveInterval = 250;
	conf.m_allocator = n_tools::createObject<n_devstone::DevstoneAlloc>();
	std::size_t width = 5;
	std::size_t depth = 5;
	bool randTa = true;

	auto ctrl = conf.createController();
	t_timestamp endTime(eTime, 0);
	ctrl->setTerminationTime(endTime);

	t_coupledmodelptr d = n_tools::createObject< n_devstone::DEVStone>(width, depth, randTa);
	ctrl->addModel(d);
	std::ofstream filestream(SUBTESTFOLDER "devstoneSingleR.txt");
	{
		CoutRedirect myRedirect(filestream);
		ctrl->simulate();
	};

	EXPECT_EQ(n_misc::filecmp(SUBTESTFOLDER "devstoneSingleR.txt", SUBTESTFOLDER "devstoneSingleR.corr"), 0);
    LOG_MOVE("out.txt", true);
}

TEST(Benchmark, devstone_opt_r)
{
    LOG_MOVE("logs/bmarkDevstoneOptR.log", false);
	n_control::ControllerConfig conf;
	conf.m_name = "DEVStone";
	conf.m_simType = n_control::SimType::OPTIMISTIC;
	conf.m_coreAmount = 4;
	conf.m_saveInterval = 250;
	conf.m_allocator = n_tools::createObject<n_devstone::DevstoneAlloc>();
	std::size_t width = 5;
	std::size_t depth = 5;
	bool randTa = true;

	auto ctrl = conf.createController();
	t_timestamp endTime(eTime, 0);
	ctrl->setTerminationTime(endTime);

	t_coupledmodelptr d = n_tools::createObject< n_devstone::DEVStone>(width, depth, randTa);
	ctrl->addModel(d);
	std::ofstream filestream(SUBTESTFOLDER "devstoneOptimisticR.txt");
	{
		CoutRedirect myRedirect(filestream);
		ctrl->simulate();
	};

	EXPECT_EQ(n_misc::filecmp(SUBTESTFOLDER "devstoneOptimisticR.txt", SUBTESTFOLDER "devstoneSingleR.corr"), 0);
    LOG_MOVE("out.txt", true);
}

TEST(Benchmark, devstone_cons_r)
{
    LOG_MOVE("logs/bmarkDevstoneConsR.log", false);
	n_control::ControllerConfig conf;
	conf.m_name = "DEVStone";
	conf.m_simType = n_control::SimType::CONSERVATIVE;
	conf.m_coreAmount = 4;
	conf.m_saveInterval = 250;
	conf.m_allocator = n_tools::createObject<n_devstone::DevstoneAlloc>();
	std::size_t width = 5;
	std::size_t depth = 5;
	bool randTa = true;

	auto ctrl = conf.createController();
	t_timestamp endTime(eTime, 0);
	ctrl->setTerminationTime(endTime);

	t_coupledmodelptr d = n_tools::createObject< n_devstone::DEVStone>(width, depth, randTa);
	ctrl->addModel(d);
	std::ofstream filestream(SUBTESTFOLDER "devstoneConservativeR.txt");
	{
		CoutRedirect myRedirect(filestream);
		ctrl->simulate();
	};

	EXPECT_EQ(n_misc::filecmp(SUBTESTFOLDER "devstoneConservativeR.txt", SUBTESTFOLDER "devstoneSingleR.corr"), 0);
    LOG_MOVE("out.txt", true);
}

constexpr t_timestamp::t_time eTimePhold = 500;

TEST(Benchmark, phold_single)
{
    LOG_MOVE("logs/bmarkPholdSingle.log", false);
	n_control::ControllerConfig conf;
	conf.m_name = "DEVStone";
	conf.m_simType = n_control::SimType::CLASSIC;
	conf.m_coreAmount = 4;
	conf.m_saveInterval = 250;
	conf.m_allocator = n_tools::createObject<n_benchmarks_phold::PHoldAlloc>();
	std::size_t nodes = 4;
	std::size_t apn = 2;
	std::size_t iter = 0;
	std::size_t percentageRemotes = 10;

	auto ctrl = conf.createController();
	t_timestamp endTime(eTimePhold, 0);
	ctrl->setTerminationTime(endTime);

	t_coupledmodelptr d = n_tools::createObject<n_benchmarks_phold::PHOLD>(nodes, apn, iter,
	        percentageRemotes);
	ctrl->addModel(d);
	std::ofstream filestream(SUBTESTFOLDER "pholdSingle.txt");
	{
		CoutRedirect myRedirect(filestream);
		ctrl->simulate();
	};

	EXPECT_EQ(n_misc::filecmp(SUBTESTFOLDER "pholdSingle.txt", SUBTESTFOLDER "pholdSingle.corr"), 0);
    LOG_MOVE("out.txt", true);
}

TEST(Benchmark, phold_opt)
{
    LOG_MOVE("logs/bmarkPholdOpt.log", false);
	n_control::ControllerConfig conf;
	conf.m_name = "PHOLD";
	conf.m_simType = n_control::SimType::OPTIMISTIC;
	conf.m_coreAmount = 4;
	conf.m_saveInterval = 250;
	conf.m_allocator = n_tools::createObject<n_benchmarks_phold::PHoldAlloc>();
	std::size_t nodes = 4;
	std::size_t apn = 2;
	std::size_t iter = 0;
	std::size_t percentageRemotes = 10;

	auto ctrl = conf.createController();
	t_timestamp endTime(eTimePhold, 0);
	ctrl->setTerminationTime(endTime);

	t_coupledmodelptr d = n_tools::createObject<n_benchmarks_phold::PHOLD>(nodes, apn, iter,
	        percentageRemotes);
	ctrl->addModel(d);
	std::ofstream filestream(SUBTESTFOLDER "pholdOptimistic.txt");
	{
		CoutRedirect myRedirect(filestream);
		ctrl->simulate();
	};

	EXPECT_EQ(n_misc::filecmp(SUBTESTFOLDER "pholdOptimistic.txt", SUBTESTFOLDER "pholdSingle.corr"), 0);
    LOG_MOVE("out.txt", true);
}

TEST(Benchmark, phold_cons)
{
    LOG_MOVE("logs/bmarkPholdCons.log", false);
	n_control::ControllerConfig conf;
	conf.m_name = "PHOLD";
	conf.m_simType = n_control::SimType::CONSERVATIVE;
	conf.m_coreAmount = 4;
	conf.m_saveInterval = 250;
	conf.m_allocator = n_tools::createObject<n_benchmarks_phold::PHoldAlloc>();
	std::size_t nodes = 4;
	std::size_t apn = 2;
	std::size_t iter = 0;
	std::size_t percentageRemotes = 10;

	auto ctrl = conf.createController();
	t_timestamp endTime(eTimePhold, 0);
	ctrl->setTerminationTime(endTime);

	t_coupledmodelptr d = n_tools::createObject<n_benchmarks_phold::PHOLD>(nodes, apn, iter,
	        percentageRemotes);
	ctrl->addModel(d);
	std::ofstream filestream(SUBTESTFOLDER "pholdConservative.txt");
	{
		CoutRedirect myRedirect(filestream);
		ctrl->simulate();
	};

	EXPECT_EQ(n_misc::filecmp(SUBTESTFOLDER "pholdConservative.txt", SUBTESTFOLDER "pholdSingle.corr"), 0);
    LOG_MOVE("out.txt", true);
}

constexpr t_timestamp::t_time eTimeConnect = 5000;

TEST(Benchmark, connect_single)
{
    LOG_MOVE("logs/bmarkConnectSingle.log", false);
	n_control::ControllerConfig conf;
	conf.m_name = "Interconnect";
	conf.m_simType = n_control::SimType::CLASSIC;
	conf.m_coreAmount = 4;
	conf.m_saveInterval = 250;
	conf.m_allocator = n_tools::createObject<n_interconnect::InterconnectAlloc>();
	std::size_t width = 4;
	bool randTa = false;

	auto ctrl = conf.createController();
	t_timestamp endTime(eTimeConnect, 0);
	ctrl->setTerminationTime(endTime);

	t_coupledmodelptr d = n_tools::createObject<n_interconnect::HighInterconnect>(width, randTa);
	ctrl->addModel(d);
	std::ofstream filestream(SUBTESTFOLDER "connectSingle.txt");
	{
		CoutRedirect myRedirect(filestream);
		ctrl->simulate();
	};

	EXPECT_EQ(n_misc::filecmp(SUBTESTFOLDER "connectSingle.txt", SUBTESTFOLDER "connectSingle.corr"), 0);
    LOG_MOVE("out.txt", true);
}

TEST(Benchmark, connect_opt)
{
    LOG_MOVE("logs/bmarkConnectOpt.log", false);
	n_control::ControllerConfig conf;
	conf.m_name = "Interconnect";
	conf.m_simType = n_control::SimType::OPTIMISTIC;
	conf.m_coreAmount = 4;
	conf.m_saveInterval = 250;
	conf.m_allocator = n_tools::createObject<n_interconnect::InterconnectAlloc>();
	std::size_t width = 4;
	bool randTa = false;

	auto ctrl = conf.createController();
	t_timestamp endTime(eTimeConnect, 0);
	ctrl->setTerminationTime(endTime);

	t_coupledmodelptr d = n_tools::createObject<n_interconnect::HighInterconnect>(width, randTa);
	ctrl->addModel(d);
	std::ofstream filestream(SUBTESTFOLDER "connectOptimistic.txt");
	{
		CoutRedirect myRedirect(filestream);
		ctrl->simulate();
	};

	EXPECT_EQ(n_misc::filecmp(SUBTESTFOLDER "connectOptimistic.txt", SUBTESTFOLDER "connectSingle.corr"), 0);
    LOG_MOVE("out.txt", true);
}

TEST(Benchmark, connect_cons)
{
    LOG_MOVE("logs/bmarkConnectCons.log", false);
	n_control::ControllerConfig conf;
	conf.m_name = "Interconnect";
	conf.m_simType = n_control::SimType::CONSERVATIVE;
	conf.m_coreAmount = 4;
	conf.m_saveInterval = 250;
	conf.m_allocator = n_tools::createObject<n_interconnect::InterconnectAlloc>();
	std::size_t width = 4;
	bool randTa = false;

	auto ctrl = conf.createController();
	t_timestamp endTime(eTimeConnect, 0);
	ctrl->setTerminationTime(endTime);

	t_coupledmodelptr d = n_tools::createObject<n_interconnect::HighInterconnect>(width, randTa);
	ctrl->addModel(d);
	std::ofstream filestream(SUBTESTFOLDER "connectConservative.txt");
	{
		CoutRedirect myRedirect(filestream);
		ctrl->simulate();
	};

	EXPECT_EQ(n_misc::filecmp(SUBTESTFOLDER "connectConservative.txt", SUBTESTFOLDER "connectSingle.corr"), 0);
    LOG_MOVE("out.txt", true);
}

TEST(Benchmark, connect_single_r)
{
    LOG_MOVE("logs/bmarkConnectSingleR.log", false);
	n_control::ControllerConfig conf;
	conf.m_name = "Interconnect";
	conf.m_simType = n_control::SimType::CLASSIC;
	conf.m_coreAmount = 4;
	conf.m_saveInterval = 250;
	conf.m_allocator = n_tools::createObject<n_interconnect::InterconnectAlloc>();
	std::size_t width = 4;
	bool randTa = true;

	auto ctrl = conf.createController();
	t_timestamp endTime(eTimeConnect, 0);
	ctrl->setTerminationTime(endTime);

	t_coupledmodelptr d = n_tools::createObject<n_interconnect::HighInterconnect>(width, randTa);
	ctrl->addModel(d);
	std::ofstream filestream(SUBTESTFOLDER "connectSingleR.txt");
	{
		CoutRedirect myRedirect(filestream);
		ctrl->simulate();
	};

	EXPECT_EQ(n_misc::filecmp(SUBTESTFOLDER "connectSingleR.txt", SUBTESTFOLDER "connectSingleR.corr"), 0);
    LOG_MOVE("out.txt", true);
}

TEST(Benchmark, connect_opt_r)
{
    LOG_MOVE("logs/bmarkConnectOptR.log", false);
	n_control::ControllerConfig conf;
	conf.m_name = "Interconnect";
	conf.m_simType = n_control::SimType::OPTIMISTIC;
	conf.m_coreAmount = 4;
	conf.m_saveInterval = 250;
	conf.m_allocator = n_tools::createObject<n_interconnect::InterconnectAlloc>();
	std::size_t width = 4;
	bool randTa = true;

	auto ctrl = conf.createController();
	t_timestamp endTime(eTimeConnect, 0);
	ctrl->setTerminationTime(endTime);

	t_coupledmodelptr d = n_tools::createObject<n_interconnect::HighInterconnect>(width, randTa);
	ctrl->addModel(d);
	std::ofstream filestream(SUBTESTFOLDER "connectOptimisticR.txt");
	{
		CoutRedirect myRedirect(filestream);
		ctrl->simulate();
	};

	EXPECT_EQ(n_misc::filecmp(SUBTESTFOLDER "connectOptimisticR.txt", SUBTESTFOLDER "connectSingleR.corr"), 0);
    LOG_MOVE("out.txt", true);
}

TEST(Benchmark, connect_cons_r)
{
    LOG_MOVE("logs/bmarkConnectConsR.log", false);
	n_control::ControllerConfig conf;
	conf.m_name = "Interconnect";
	conf.m_simType = n_control::SimType::CONSERVATIVE;
	conf.m_coreAmount = 4;
	conf.m_saveInterval = 250;
	conf.m_allocator = n_tools::createObject<n_interconnect::InterconnectAlloc>();
	std::size_t width = 4;
	bool randTa = true;

	auto ctrl = conf.createController();
	t_timestamp endTime(eTimeConnect, 0);
	ctrl->setTerminationTime(endTime);

	t_coupledmodelptr d = n_tools::createObject<n_interconnect::HighInterconnect>(width, randTa);
	ctrl->addModel(d);
	std::ofstream filestream(SUBTESTFOLDER "connectConservativeR.txt");
	{
		CoutRedirect myRedirect(filestream);
		ctrl->simulate();
	};

	EXPECT_EQ(n_misc::filecmp(SUBTESTFOLDER "connectConservativeR.txt", SUBTESTFOLDER "connectSingleR.corr"), 0);

    LOG_MOVE("out.txt", true);
}
