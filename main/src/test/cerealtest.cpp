/*
 * cerealtest.cpp
 *
 *  Created on: Apr 12, 2015
 *      Author: pieter
 */

#include <gtest/gtest.h>
#include "port.h"
#include <cereal/types/map.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/memory.hpp>
#include <cereal/types/unordered_map.hpp>
#include <cereal/archives/binary.hpp>
#include <sstream>

using namespace n_model;

struct MyRecord
{
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
