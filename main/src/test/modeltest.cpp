/*
 * modeltest.cpp
 *
 *  Created on: Mar 22, 2015
 *      Author: tim, matthijs
 */

#include <gtest/gtest.h>
#include "model.h"
#include "objectfactory.h"
#include "atomicmodel.h"
#include "rootmodel.h"
#include "trafficlight.h"
#include "port.h"
#include "coupledmodel.h"
#include "trafficsystemc.h"

using namespace n_model;
using namespace n_tools;

/*
 * Example models derived from the Policeman example
 */
class PoliceBoss: public n_examples_coupled::Policeman
{
public:
	PoliceBoss()
		: n_examples_coupled::Policeman("policeBoss")
	{
		addOutPort("POLICECOMMSOUT");
	}
	virtual ~PoliceBoss()
	{
	}
};
class PoliceOfficer: public n_examples_coupled::Policeman
{
public:
	PoliceOfficer(std::string name)
		: n_examples_coupled::Policeman(name)
	{
		addInPort("POLICECOMMSIN");
	}
	virtual ~PoliceOfficer()
	{
	}
};
class PoliceSystem: public CoupledModel
{
public:
	PoliceSystem() : CoupledModel("PoliceSystem")
	{
		addInPort("IN");
		t_atomicmodelptr policeOfficer = createObject<PoliceOfficer>("policeOfficer");
		t_atomicmodelptr trafficlight = createObject<n_examples_coupled::TrafficLight>("trafficlight");
		addSubModel(policeOfficer);
		addSubModel(trafficlight);
		connectPorts(policeOfficer->getPort("OUT"), trafficlight->getPort("INTERRUPT"));
		connectPorts(getPort("IN"), policeOfficer->getPort("POLICECOMMSIN"));
	}
	virtual ~PoliceSystem()
	{}

};

/*
 * Simple example of a layered coupled model
 */
class LayeredCoupled : public CoupledModel
{
public:
	LayeredCoupled(std::string name) : CoupledModel(name)
	{
		/*
		 *  ------------ LayeredCoupled -----------
		 * |                                       |
		 * |    policeBoss                         |
		 * |       |                               |
		 * |       v            	           |
		 * |  ----------- PoliceSystem ----------  |
		 * | |     |                             | |
		 * | |     v                             | |
		 * | |  policeOfficer --> trafficlight   | |
		 * | |                                   | |
		 * | |                                   | |
		 * |  ===================================  |
		 * |                                       |
		 *  =======================================
		 */

		t_atomicmodelptr policeBoss = createObject<PoliceBoss>();
		addSubModel(policeBoss);
		t_coupledmodelptr policeSystem = createObject<PoliceSystem>();
		addSubModel(policeSystem);
		connectPorts(policeBoss->getPort("POLICECOMMSOUT"), policeSystem->getPort("IN"));
	}
	virtual ~LayeredCoupled() {}
};

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
	n_examples::TrafficLight tl("Trafficlight1");
	EXPECT_EQ(tl.getPriority(), 0);
	EXPECT_EQ(tl.getName(), "Trafficlight1");
}

TEST(Model, TransitionTesting)
{
	RecordProperty("description", "Verifies transition functionality of models using the TrafficLightModel");
	n_examples::TrafficLight tl("TrafficLight1");
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
	n_examples::TrafficLightMode mode("red");
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

TEST(RootModel, DirectConnectLayered)
{
	RecordProperty("description", "Tests directConnect with a layered coupled model");
	std::shared_ptr<RootModel> root = createObject<RootModel>();
	t_coupledmodelptr example = createObject<LayeredCoupled>("LayeredCoupled");
	std::vector<t_atomicmodelptr> atomics = root->directConnect(example);

	EXPECT_TRUE(atomics.size() == 3);
	for (auto a : atomics) {
		std::string name = a->getName();
		if (name == "policeBoss") {
			t_portptr commsOut = a->getPort("POLICECOMMSOUT");
			EXPECT_TRUE(commsOut != nullptr);
			EXPECT_TRUE(commsOut->isUsingDirectConnect());
			std::map<t_portptr, std::vector<t_zfunc> > outs = commsOut->getCoupledOuts();
			EXPECT_TRUE(!outs.empty());
			t_portptr commsIn = outs.begin()->first;
			EXPECT_EQ(commsIn->getName(), "POLICECOMMSIN");
		} else if (name == "policeOfficer") {
			t_portptr commsIn = a->getPort("POLICECOMMSIN");
			EXPECT_TRUE(commsIn != nullptr);
			EXPECT_TRUE(commsIn->isUsingDirectConnect());
			auto ins = commsIn->getCoupledIns();
			EXPECT_TRUE(!ins.empty());
			t_portptr commsOut = ins[0];
			EXPECT_EQ(commsOut->getName(), "POLICECOMMSOUT");
		}
	}
}
