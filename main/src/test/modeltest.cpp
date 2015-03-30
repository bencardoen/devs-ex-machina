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
#include "port.h"
#include "coupledmodel.h"
#include "trafficsystemc.h"

using namespace n_model;
using namespace n_examples;

TEST(Model, Basic)
{
	RecordProperty("description", "Verifies all basic functionality of the default model functions");
	Model model1("ABC");
	EXPECT_EQ(model1.getState(), nullptr);
	EXPECT_EQ(model1.getIPorts().size(), 0);
	EXPECT_EQ(model1.getOPorts().size(), 0);
	EXPECT_EQ(model1.getSendMessages().size(), 0);
	EXPECT_EQ(model1.getReceivedMessages().size(), 0);
	EXPECT_EQ(model1.getName(), "ABC");
	EXPECT_EQ(model1.getPort(""), nullptr);
	Model model2("DEF");
	EXPECT_EQ(model2.getName(), "DEF");
}

TEST(Model, AtomicModel)
{
	RecordProperty("description", "Verifies all basic functionality of AtomicModel");
	TrafficLight tl("Trafficlight1");
	EXPECT_EQ(tl.getPriority(), 0);
	EXPECT_EQ(tl.getName(), "Trafficlight1");
}

TEST(Model, TransitionTesting)
{
	RecordProperty("description", "Verifies transition functionality of models using the TrafficLightModel");
	TrafficLight tl("TrafficLight1");
	EXPECT_EQ(tl.getState()->toString(), "red");
	tl.setTime(t_timestamp(30));
	EXPECT_EQ(tl.timeAdvance(), t_timestamp(60));
	EXPECT_EQ(tl.getState()->m_timeLast, t_timestamp(30));
	EXPECT_EQ(tl.getState()->m_timeNext, t_timestamp(90));
	tl.intTransition();
	EXPECT_EQ(tl.getState()->toString(), "green");
	EXPECT_EQ(tl.timeAdvance(), t_timestamp(50));
	tl.intTransition();
	EXPECT_EQ(tl.getState()->toString(), "yellow");
	EXPECT_EQ(tl.timeAdvance(), t_timestamp(10));
	tl.intTransition();
	EXPECT_EQ(tl.getState()->toString(), "red");
	EXPECT_EQ(tl.timeAdvance(), t_timestamp(60));
}

TEST(State, Basic)
{
	RecordProperty("description", "Verifies bassic functionality of state");
	TrafficLightMode mode("red");
	EXPECT_EQ(mode.toString(), "red");
	EXPECT_EQ(mode.toXML(), "<state color =\"red\"/>");
	EXPECT_EQ(mode.toJSON(), "{ \"state\": \"red\" }");
	EXPECT_TRUE(mode == "red");
	EXPECT_TRUE("red" == mode);
}

TEST(Port, Basic)
{
	n_examples_coupled::TrafficSystem trafficsystem("trafficsystem");
	auto components = trafficsystem.getComponents();
	t_modelptr policeman = components.at(0);
	t_modelptr trafficlight = components.at(1);
	EXPECT_TRUE(policeman->getPort("OUT") != nullptr);
	EXPECT_TRUE(trafficlight->getPort("INTERRUPT") != nullptr);
}

