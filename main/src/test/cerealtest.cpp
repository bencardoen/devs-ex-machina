/*
 * cerealtest.cpp
 *
 *  Created on: Apr 12, 2015
 *      Author: pieter
 */

#include <gtest/gtest.h>
#include "port.h"
#include "state.h"
#include "zfunc.h"
#include "message.h"
#include "model.h"
#include "atomicmodel.h"
#include "coupledmodel.h"
#include "core.h"
#include "modelentry.h"
#include "messageentry.h"
#include "trafficsystemc.h"
#include "cereal/archives/binary.hpp"
#include "cereal/types/polymorphic.hpp"
#include <sstream>
#include "cereal/types/vector.hpp"

using namespace n_model;
using namespace n_network;
using namespace n_examples_coupled;

class MyRecord
{
public:
  uint8_t x, y;
  float z;

  template <class Archive>
  void serialize( Archive & ar )
  {
    ar( x, y, z );
  }
};

TEST(Cereal, General)
{
	std::stringstream ss;

	MyRecord recOut;
	recOut.x = 5;
	recOut.y = 6;
	recOut.z = 3.33;

	MyRecord recIn;

	cereal::BinaryOutputArchive oarchive(ss);
	cereal::BinaryInputArchive iarchive(ss);

	oarchive(recOut);
	iarchive(recIn);

	EXPECT_EQ(recOut.x, recIn.x);
	EXPECT_EQ(recOut.y, recIn.y);
	EXPECT_EQ(recOut.z, recIn.z);
}

TEST(Cereal, PortBasic)
{
	std::stringstream ss;

	Port po1 = Port("poort1", "host1", true);
	Port po2 = Port("err", "err", false);

	cereal::BinaryOutputArchive oarchive(ss);
	cereal::BinaryInputArchive iarchive(ss);

	oarchive(po1);
	iarchive(po2);

	EXPECT_EQ(po1.getFullName(), po2.getFullName());
	EXPECT_EQ(po1.isInPort(), po2.isInPort());
}

TEST(Cereal, Zfunc)
{
	std::stringstream ss;

	ZFunc z1;
	ZFunc z2;

	cereal::BinaryOutputArchive oarchive(ss);
	cereal::BinaryInputArchive iarchive(ss);

	oarchive(z1);
	iarchive(z2);

	t_msgptr m0 = std::make_shared<n_network::Message>("test", 0, "dest", "source");
	t_msgptr m1, m2;
	m1 = z1(m0);
	m2 = z2(m0);
	EXPECT_TRUE(m1 == m2);
}

TEST(Cereal, MessageBasic)
{
	std::stringstream ss;

	t_msgptr m1 = std::make_shared<n_network::Message>("test", 0, "dest", "source");
	t_msgptr m2 = std::make_shared<n_network::Message>("err", 1, "err", "err");

	cereal::BinaryOutputArchive oarchive(ss);
	cereal::BinaryInputArchive iarchive(ss);

	oarchive(m1);
	iarchive(m2);

	EXPECT_TRUE(*m1 == *m2);
}

TEST(Cereal, TimeStamp)
{
	std::stringstream ss;

	t_timestamp t1 = t_timestamp(2);
	t_timestamp t2 = t_timestamp();

	cereal::BinaryOutputArchive oarchive(ss);
	cereal::BinaryInputArchive iarchive(ss);

	oarchive(t1);
	iarchive(t2);

	EXPECT_EQ(t1, t2);
}

TEST(Cereal, State)
{
	std::stringstream ss;

	State s1 = State("test");
	s1.m_timeLast = t_timestamp(42,42);
	s1.m_timeNext = t_timestamp(41,41);

	State s2 = State("err");

	cereal::BinaryOutputArchive oarchive(ss);
	cereal::BinaryInputArchive iarchive(ss);

	oarchive(s1);
	iarchive(s2);

	EXPECT_EQ(s1.toString(), s2.toString());
	EXPECT_EQ(s1.m_timeLast, s2.m_timeLast);
	EXPECT_EQ(s1.m_timeNext, s2.m_timeNext);
}

TEST(Cereal, StatePointer)
{
	std::stringstream ss;

	t_stateptr sp1 = n_tools::createObject<State>("Mystate");
	sp1->m_timeLast = t_timestamp(32,32);
	sp1->m_timeNext = t_timestamp(55,55);
	State s1 = *sp1;	// for debugging

	t_stateptr sp2 = n_tools::createObject<State>("OtherMystate");
	sp2->m_timeLast=t_timestamp(44,44);
	State s2 = *sp2;	// for debugging

	cereal::BinaryOutputArchive oarchive(ss);
	cereal::BinaryInputArchive iarchive(ss);

	oarchive(sp1);
	iarchive(sp2);

	s2 = *sp2;			// for debugging

	EXPECT_EQ(sp1->toString(), sp2->toString());
	EXPECT_EQ(sp1->m_timeLast, sp2->m_timeLast);
	EXPECT_EQ(sp1->m_timeNext, sp2->m_timeNext);
}

class TestCereal {
public:
	static void testCerealModel()
	{
		std::stringstream ss;

		Model m1 = Model("test");
		m1.m_timeLast = t_timestamp(42,42);
		m1.m_timeNext = t_timestamp(42,42);
		Model m2 = Model("err");

		cereal::BinaryOutputArchive oarchive(ss);
		cereal::BinaryInputArchive iarchive(ss);

		oarchive(m1);
		iarchive(m2);

		EXPECT_EQ(m1.getName(), m2.getName());
		EXPECT_EQ(m1.m_timeLast, m2.m_timeLast);
		EXPECT_EQ(m1.m_timeNext, m2.m_timeNext);
	}

	static void testCerealModelPointer()
	{
		std::stringstream ss;

		t_modelptr mp1 = n_tools::createObject<Model>("test");
		mp1->m_timeLast = t_timestamp(42,42);
		mp1->m_timeNext = t_timestamp(42,42);
		t_modelptr mp2 = n_tools::createObject<Model>("err");

		cereal::BinaryOutputArchive oarchive(ss);
		cereal::BinaryInputArchive iarchive(ss);

		oarchive(mp1);
		iarchive(mp2);

		EXPECT_EQ(mp1->getName(), mp2->getName());
		EXPECT_EQ(mp1->m_timeLast, mp2->m_timeLast);
		EXPECT_EQ(mp1->m_timeNext, mp2->m_timeNext);
	}

	static void testCerealMultipleModelPointers()
	{
		std::stringstream ss;

		t_modelptr mp1a = n_tools::createObject<Model>("test");
		mp1a->m_timeLast = t_timestamp(11,12);
		mp1a->m_timeNext = t_timestamp(13,14);
		t_modelptr mp1b = n_tools::createObject<Model>("err");

		t_modelptr mp2a = n_tools::createObject<Model>("test");
		mp2a->m_timeLast = t_timestamp(21,22);
		mp2a->m_timeNext = t_timestamp(23,24);
		t_modelptr mp2b = n_tools::createObject<Model>("err");

		t_modelptr mp3a = n_tools::createObject<Model>("test");
		mp3a->m_timeLast = t_timestamp(31,32);
		mp3a->m_timeNext = t_timestamp(33,34);
		t_modelptr mp3b = n_tools::createObject<Model>("err");

		cereal::BinaryOutputArchive oarchive(ss);
		cereal::BinaryInputArchive iarchive(ss);

		oarchive(mp1a, mp2a);
		oarchive(mp3a);
		iarchive(mp1b, mp2b);
		iarchive(mp3b);

		EXPECT_EQ(mp1a->getName(), mp1b->getName());
		EXPECT_EQ(mp1a->m_timeLast, mp1b->m_timeLast);
		EXPECT_EQ(mp1a->m_timeNext, mp1b->m_timeNext);

		EXPECT_EQ(mp2a->getName(), mp2b->getName());
		EXPECT_EQ(mp2a->m_timeLast, mp2b->m_timeLast);
		EXPECT_EQ(mp2a->m_timeNext, mp2b->m_timeNext);

		EXPECT_EQ(mp3a->getName(), mp3b->getName());
		EXPECT_EQ(mp3a->m_timeLast, mp3b->m_timeLast);
		EXPECT_EQ(mp3a->m_timeNext, mp3b->m_timeNext);
	}

	static void testCerealCoupledModel()
	{
		std::stringstream ss;

		CoupledModel m1 = CoupledModel("test");
		m1.m_timeLast = t_timestamp(42,42);
		m1.m_timeNext = t_timestamp(42,42);
		CoupledModel m2 = CoupledModel("err");

		cereal::BinaryOutputArchive oarchive(ss);
		cereal::BinaryInputArchive iarchive(ss);

		oarchive(m1);
		iarchive(m2);

		EXPECT_EQ(m1.getName(), m2.getName());
		EXPECT_EQ(m1.m_timeLast, m2.m_timeLast);
		EXPECT_EQ(m1.m_timeNext, m2.m_timeNext);
	}

	static void testCerealModelCoupledPointer()
	{
		std::stringstream ss;

		t_modelptr mp1 = n_tools::createObject<CoupledModel>("test");
		mp1->m_timeLast = t_timestamp(42,42);
		mp1->m_timeNext = t_timestamp(42,42);
		t_modelptr mp2 = n_tools::createObject<CoupledModel>("err");

		cereal::BinaryOutputArchive oarchive(ss);
		cereal::BinaryInputArchive iarchive(ss);

		oarchive(mp1);
		iarchive(mp2);

		EXPECT_EQ(mp1->getName(), mp2->getName());
		EXPECT_EQ(mp1->m_timeLast, mp2->m_timeLast);
		EXPECT_EQ(mp1->m_timeNext, mp2->m_timeNext);
	}

	static void testCerealAtomicModel()
	{
		std::stringstream ss;

		AtomicModel m1 = AtomicModel("test");
		m1.m_timeLast = t_timestamp(42,42);
		m1.m_timeNext = t_timestamp(42,42);
		AtomicModel m2 = AtomicModel("err");

		cereal::BinaryOutputArchive oarchive(ss);
		cereal::BinaryInputArchive iarchive(ss);

		oarchive(m1);
		iarchive(m2);

		EXPECT_EQ(m1.getName(), m2.getName());
		EXPECT_EQ(m1.m_timeLast, m2.m_timeLast);
		EXPECT_EQ(m1.m_timeNext, m2.m_timeNext);
	}

	static void testCerealAtomicModelPointer()
	{
		std::stringstream ss;

		t_atomicmodelptr mp1 = n_tools::createObject<AtomicModel>("test");
		mp1->m_timeLast = t_timestamp(42,42);
		mp1->m_timeNext = t_timestamp(42,42);
		t_atomicmodelptr mp2 = n_tools::createObject<AtomicModel>("err");

		cereal::BinaryOutputArchive oarchive(ss);
		cereal::BinaryInputArchive iarchive(ss);

		oarchive(mp1);
		iarchive(mp2);

		EXPECT_EQ(mp1->getName(), mp2->getName());
		EXPECT_EQ(mp1->m_timeLast, mp2->m_timeLast);
		EXPECT_EQ(mp1->m_timeNext, mp2->m_timeNext);
	}

	static void testCerealPort()
	{
		std::stringstream ss;

		Port p1 = Port("poort1", "host1", true);
		p1.m_usingDirectConnect = true;
		Port p2 = Port("err", "err", false);
		p2.m_usingDirectConnect = false;

		cereal::BinaryOutputArchive oarchive(ss);
		cereal::BinaryInputArchive iarchive(ss);

		oarchive(p1);
		iarchive(p2);

		EXPECT_EQ(p1.getFullName(), p2.getFullName());
		EXPECT_EQ(p1.isInPort(), p2.isInPort());
		EXPECT_EQ(p1.m_usingDirectConnect, p2.m_usingDirectConnect);
	}

	static void testCerealPortPointer()
	{
		std::stringstream ss;

		t_portptr pp1 = n_tools::createObject<Port>("poort1", "host1", true);
		pp1->m_usingDirectConnect = true;
		t_portptr pp2 = n_tools::createObject<Port>("err", "err", false);
		pp2->m_usingDirectConnect = false;

		cereal::BinaryOutputArchive oarchive(ss);
		cereal::BinaryInputArchive iarchive(ss);

		oarchive(pp1);
		iarchive(pp2);

		EXPECT_EQ(pp1->getFullName(), pp2->getFullName());
		EXPECT_EQ(pp1->isInPort(), pp2->isInPort());
		EXPECT_EQ(pp1->m_usingDirectConnect, pp2->m_usingDirectConnect);
	}

	static void testCerealMessagePointer()
	{
		std::stringstream ss;

		t_msgptr mp1 = n_tools::createObject<Message>("test", 0, "dest", "source");
		mp1->m_timestamp = t_timestamp(42,42);
		mp1->m_antimessage = true;
		t_msgptr mp2 = n_tools::createObject<Message>("err", 1, "err", "err");

		cereal::BinaryOutputArchive oarchive(ss);
		cereal::BinaryInputArchive iarchive(ss);

		oarchive(mp1);
		iarchive(mp2);

		EXPECT_TRUE(*mp1 == *mp2);
		EXPECT_EQ(mp1->m_timestamp, mp2->m_timestamp);
		EXPECT_EQ(mp1->m_antimessage, mp2->m_antimessage);
	}
};

TEST(Cereal, Model)
{
	TestCereal::testCerealModel();
}

TEST(Cereal, ModelPointer)
{
	TestCereal::testCerealModelPointer();
}

TEST(Cereal, MultipleModelPointers)
{
	TestCereal::testCerealMultipleModelPointers();
}

TEST(Cereal, CoupledModel)
{
	TestCereal::testCerealCoupledModel();
}

TEST(Cereal, CoupledModelPointer)
{
	TestCereal::testCerealModelCoupledPointer();
}

TEST(Cereal, AtomicModel)
{
	TestCereal::testCerealAtomicModel();
}

TEST(Cereal, AtomicModelPointer)
{
	TestCereal::testCerealAtomicModelPointer();
}

TEST(Cereal, Port)
{
	TestCereal::testCerealPort();
}

TEST(Cereal, PortPointer)
{
	TestCereal::testCerealPortPointer();
}

TEST(Cereal, MessagePointer)
{
	TestCereal::testCerealMessagePointer();
}

TEST(Cereal, AtomicModelBasic)
{
	std::stringstream ss;

	AtomicModel m1 = AtomicModel("test");
	AtomicModel m2 = AtomicModel("err");

	cereal::BinaryOutputArchive oarchive(ss);
	cereal::BinaryInputArchive iarchive(ss);

	oarchive(m1);
	iarchive(m2);

	EXPECT_EQ(m1.getName(), m2.getName());
}

TEST(Cereal, CoupledModelBasic)
{
	std::stringstream ss;

	CoupledModel m1 = CoupledModel("test");
	CoupledModel m2 = CoupledModel("err");

	cereal::BinaryOutputArchive oarchive(ss);
	cereal::BinaryInputArchive iarchive(ss);

	oarchive(m1);
	iarchive(m2);

	EXPECT_EQ(m1.getName(), m2.getName());
}

TEST(Cereal, ModelPolyModel)
{
	std::stringstream ss;

	t_modelptr mpm1 = std::make_shared<Model>("test");
	t_modelptr mpm2 = std::make_shared<Model>("err");

	cereal::BinaryOutputArchive oarchive(ss);
	cereal::BinaryInputArchive iarchive(ss);

	oarchive(mpm1);
	iarchive(mpm2);

	EXPECT_EQ(mpm1->getName(), mpm2->getName());
}

TEST(Cereal, ModelPolyAtomicModel)
{
	std::stringstream ss;

	t_modelptr mpa1 = std::make_shared<AtomicModel>("test");
	t_modelptr mpa2 = std::make_shared<AtomicModel>("err");

	cereal::BinaryOutputArchive oarchive(ss);
	cereal::BinaryInputArchive iarchive(ss);

	oarchive(mpa1);
	iarchive(mpa2);

	EXPECT_EQ(mpa1->getName(), mpa2->getName());
}

TEST(Cereal, ModelPolyCoupledModel)
{
	std::stringstream ss;

	t_modelptr mpc1 = std::make_shared<CoupledModel>("test");
	t_modelptr mpc2 = std::make_shared<CoupledModel>("err");

	cereal::BinaryOutputArchive oarchive(ss);
	cereal::BinaryInputArchive iarchive(ss);

	oarchive(mpc1);
	iarchive(mpc2);

	EXPECT_EQ(mpc1->getName(), mpc2->getName());
}

TEST(Cereal, Core)
{
	Core c1;
	std::string filename ("core1_test_out.bin");
	c1.save(filename);

	Core c2;
	c2.load(filename);

	EXPECT_EQ(c1.getCoreID(), c2.getCoreID());
}

TEST(Cereal, MessageEntry)
{
	std::stringstream ss;

	t_msgptr m1 = std::make_shared<n_network::Message>("test", 0, "dest", "source");
	t_msgptr m2 = std::make_shared<n_network::Message>("err", 1, "err", "err");

	MessageEntry me1 (m1);
	MessageEntry me2 (m2);

	cereal::BinaryOutputArchive oarchive(ss);
	cereal::BinaryInputArchive iarchive(ss);

	oarchive(me1);
	iarchive(me2);

	EXPECT_TRUE(me1 == me2);
}

TEST(Cereal, ModelEntry)
{
	std::stringstream ss;

	ModelEntry me1 ("test", 0);
	ModelEntry me2 ("err", 0);

	cereal::BinaryOutputArchive oarchive(ss);
	cereal::BinaryInputArchive iarchive(ss);

	oarchive(me1);
	iarchive(me2);

	EXPECT_TRUE(me1 == me2);
}

TEST(Cereal, ExampleModels)
{
	{
		std::stringstream ss;

		t_modelptr mp1o = n_tools::createObject<TrafficLight>("TrafficLight");
		t_modelptr mp1i  = n_tools::createObject<TrafficLight>("Err");
		t_modelptr mp2o = n_tools::createObject<Policeman>("Policeman");
		t_modelptr mp2i = n_tools::createObject<Policeman>("Err");
		t_modelptr mp3o = n_tools::createObject<Policeman>("Policeman");
		t_modelptr mp3i = n_tools::createObject<Policeman>("Err");
		int a = 99;
		int b;

		//int c = 199;
		//int d = 0;

		cereal::BinaryOutputArchive oarchive(ss);
		cereal::BinaryInputArchive iarchive(ss);

		std::vector<t_modelptr> vector;
		vector.push_back(mp1o);
		vector.push_back(mp2o);

		std::vector<t_modelptr> vector2;
		std::vector<int> intv;
		intv.push_back(5);
		intv.push_back(3);

		oarchive(mp1o);
		oarchive(mp3o);
		oarchive(a);
		iarchive(mp1i);
		iarchive(mp3i);
		iarchive(b);

		//EXPECT_EQ(mp1o->getName(), mp1i->getName());
		//EXPECT_EQ(vector.at(0)->getName(), vector2.at(0)->getName());
		//EXPECT_EQ(vector.at(1)->getName(), vector2.at(1)->getName());
		//EXPECT_EQ(vector.at(1)->getName(), vector2.at(1)->getName());
		//EXPECT_EQ(mp2o->getName(), mp2i->getName());
		//EXPECT_EQ(a, b);
		EXPECT_EQ(mp1o->getName(), mp1i->getName());
	}
	{
		std::stringstream ss;
		t_modelptr mp2o = std::make_shared<Policeman>("Policeman");
		t_modelptr mp2i;

		cereal::BinaryOutputArchive oarchive(ss);
		cereal::BinaryInputArchive iarchive(ss);

		oarchive(mp2o);
		iarchive(mp2i);

		EXPECT_EQ(mp2o->getName(), mp2i->getName());
	}

	{
		std::stringstream ss;
		t_modelptr mp3o = std::make_shared<TrafficSystem>("TrafficSystem");
		t_modelptr mp3i;

		cereal::BinaryOutputArchive oarchive(ss);
		cereal::BinaryInputArchive iarchive(ss);

		oarchive(mp3o);
		iarchive(mp3i);

		EXPECT_EQ(mp3o->getName(), mp3i->getName());
	}
}
