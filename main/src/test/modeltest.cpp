/*
 * modeltest.cpp
 *
 *  Created on: Mar 22, 2015
 *      Author: tim
 */

#include <gtest/gtest.h>
#include "model.h"
#include "atomicmodel.h"
#include "trafficlight.h"

using namespace n_model;
using namespace n_examples;

TEST(Model, Basic) {
	RecordProperty("description", "Verifies all basic functionality of the default model functions");
	Model model1("ABC", 0);
	EXPECT_EQ(model1.getState(), nullptr);
	EXPECT_EQ(model1.getIPorts().size(), 0);
	EXPECT_EQ(model1.getOPorts().size(), 0);
	EXPECT_EQ(model1.getSendMessages().size(), 0);
	EXPECT_EQ(model1.getReceivedMessages().size(), 0);
	EXPECT_EQ(model1.getName(), "ABC");
	EXPECT_EQ(model1.getPort(""), nullptr);
	EXPECT_EQ(model1.getCoreNumber(), 0);
	model1.setCoreNumber(1);
	EXPECT_EQ(model1.getCoreNumber(), 1);
	Model model2("DEF", 5);
	EXPECT_EQ(model2.getName(), "DEF");
	EXPECT_EQ(model2.getCoreNumber(), 5);
}

TEST(Model, AtomicModel) {
	RecordProperty("description", "Verifies all basic functionality of AtomicModel");
	TrafficLight tl("Trafficlight1");
	EXPECT_EQ(tl.getPriority(), 0);
	EXPECT_EQ(tl.getName(), "Trafficlight1");
}

TEST(Model, TransitionTesting) {
	RecordProperty("description", "Verifies transition functionality of models using the TrafficLightModel");
	TrafficLight tl("TrafficLight1");
	EXPECT_EQ(tl.getState()->toString(), "Red");
	EXPECT_EQ(tl.timeAdvance(), t_timestamp(60));
	tl.intTransition();
	EXPECT_EQ(tl.getState()->toString(), "Green");
	EXPECT_EQ(tl.timeAdvance(), t_timestamp(50));
	tl.intTransition();
	EXPECT_EQ(tl.getState()->toString(), "Yellow");
	EXPECT_EQ(tl.timeAdvance(), t_timestamp(10));
	tl.intTransition();
	EXPECT_EQ(tl.getState()->toString(), "Red");
	EXPECT_EQ(tl.timeAdvance(), t_timestamp(60));
}

TEST(State, Basic) {
	RecordProperty("description", "Verifies bassic functionality of state");
	TrafficLightMode mode("Red");
	EXPECT_EQ(mode.toString(), "Red");
	EXPECT_EQ(mode.toXML(), "<state color =\"Red\"/>");
	EXPECT_EQ(mode.toJSON(), "{ \"state\": \"Red\" }");
	EXPECT_TRUE(mode == "Red");
	EXPECT_TRUE("Red" == mode);
}


