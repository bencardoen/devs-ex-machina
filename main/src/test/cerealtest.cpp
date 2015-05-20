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
#include "cereal/archives/binary.hpp"
#include "cereal/types/polymorphic.hpp"
#include <sstream>

using namespace n_model;
using namespace n_network;

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

TEST(Cereal, Port)
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

TEST(Cereal, Message)
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
	State s2 = State("err");

	cereal::BinaryOutputArchive oarchive(ss);
	cereal::BinaryInputArchive iarchive(ss);

	oarchive(s1);
	iarchive(s2);

	EXPECT_EQ(s1.toString(), s2.toString());
}

TEST(Cereal, Model)
{
	std::stringstream ss;

	Model m1 = Model("test");
	Model m2 = Model("err");

	cereal::BinaryOutputArchive oarchive(ss);
	cereal::BinaryInputArchive iarchive(ss);

	oarchive(m1);
	iarchive(m2);

	EXPECT_EQ(m1.getName(), m2.getName());
}

TEST(Cereal, AtomicModel)
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

TEST(Cereal, CoupledModel)
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

	t_modelptr mpm1 = std::make_shared<AtomicModel>("test");
	t_modelptr mpm2 = std::make_shared<AtomicModel>("err");

	cereal::BinaryOutputArchive oarchive(ss);
	cereal::BinaryInputArchive iarchive(ss);

	oarchive(mpm1);
	iarchive(mpm2);

	EXPECT_EQ(mpm1->getName(), mpm2->getName());
}

TEST(Cereal, ModelPolyCoupledModel)
{
	std::stringstream ss;

	t_modelptr mpm1 = std::make_shared<CoupledModel>("test");
	t_modelptr mpm2 = std::make_shared<CoupledModel>("err");

	cereal::BinaryOutputArchive oarchive(ss);
	cereal::BinaryInputArchive iarchive(ss);

	oarchive(mpm1);
	iarchive(mpm2);

	EXPECT_EQ(mpm1->getName(), mpm2->getName());
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

