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
#include <vector>
#include <chrono>

using namespace n_model;
using namespace n_tools;

TEST(Core, BasicUnitTest)
{
	t_coreptr mycore = createObject<Core>();
	//t_modelptr model = createObject();
	//mycore->
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
	EXPECT_TRUE(me < you);
	set.insert(me);
	set.insert(you);
	EXPECT_EQ(set.size(), 2);
}
