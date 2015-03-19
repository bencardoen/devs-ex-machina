/*
 * networktest.cpp
 *
 *  Created on: 9 Mar 2015
 *      Author: Ben Cardoen
 */

#include <gtest/gtest.h>
#include "timestamp.h"
#include "network.h"
#include "objectfactory.h"
#include "core.h"
#include <unordered_set>
#include <thread>
#include <sstream>
#include <vector>
#include <chrono>

using namespace n_model;
using namespace n_tools;

TEST(Core, BasicUnitTest)
{
	t_coreptr mycore = createObject<Core>();
	t_modelptr model = createObject<modelstub>("amodel");
	mycore->addModel(model);
	EXPECT_EQ(mycore->getModel("amodel"), model);
	mycore->scheduleModel("amodel", t_timestamp(0,0));
}

TEST(ModelEntry, Scheduling){
	auto scheduler = n_tools::SchedulerFactory<ModelEntry>::makeScheduler(n_tools::Storage::BINOMIAL, false);
	EXPECT_TRUE(scheduler->empty());
	std::stringstream s;
	for(size_t i = 0; i<100; ++i){
		s << i;
		std::string name = s.str();
		s.str(std::string(""));
		scheduler->push_back(ModelEntry(name, t_timestamp(i, 0)));
		EXPECT_EQ(scheduler->size(), i+1);
	}
	std::vector<ModelEntry> imminent;
	ModelEntry token ("", t_timestamp(50, 0));
	scheduler->unschedule_until(imminent, token);
	EXPECT_EQ(scheduler->size(), 50);
	token = ModelEntry("", t_timestamp(100,0));
	scheduler->unschedule_until(imminent, token);
	EXPECT_EQ(scheduler->size(), 0);
}

TEST(ModelScheduling, BasicOperations){
	ModelEntry me("alone", t_timestamp(0,0));
	ModelEntry you("home", t_timestamp(0,0));
	EXPECT_FALSE(me == you);
	std::unordered_set<ModelEntry> set;
	set.insert(me);
	set.insert(you);
	EXPECT_EQ(set.size(),2);
	set.clear();
	EXPECT_EQ(set.size(),0);
	me = ModelEntry("alone", t_timestamp(1,0));
	you = ModelEntry("alone", t_timestamp(1,1));
	EXPECT_FALSE(me == you);
	EXPECT_TRUE(me > you);
	set.insert(me);
	set.insert(you);
	EXPECT_EQ(set.size(), 2);
}
