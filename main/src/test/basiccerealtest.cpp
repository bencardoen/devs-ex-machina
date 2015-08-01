/*
 * basiccearealtest.cpp
 *
 *  Created on: May 19, 2015
 *      Author: pieter
 */

// any archives included prior to 'myclasses.hpp'
// would also apply to the registration
#include "test/basiccerealtestclasses.h"
#include <gtest/gtest.h>
#include "cereal/archives/portable_binary.hpp"
#include "cereal/types/polymorphic.hpp"

#include <iostream>
#include <fstream>

TEST(BasicCereal, Polymorphism)
{
  {
    std::ofstream os( "polymorphism_test.bin" );
    //cereal::XMLOutputArchive oarchive( os );
    //cereal::BinaryOutputArchive oarchive( os );
    cereal::PortableBinaryOutputArchive oarchive( os );

    // Create instances of the derived classes, but only keep base class pointers
    std::shared_ptr<BaseClass> ptr1 = std::make_shared<DerivedClassOne>();
    std::shared_ptr<BaseClass> ptr2 = std::make_shared<EmbarrassingDerivedClass>();
    oarchive( ptr1, ptr2 );
  }

  /*{
    std::ifstream is( "polymorphism_test.bin" );
    cereal::XMLInputArchive iarchive( is );

    // De-serialize the data as base class pointers, and watch as they are
    // re-instantiated as derived classes
    std::shared_ptr<BaseClass> ptr1;
    std::shared_ptr<BaseClass> ptr2;
    iarchive( ptr1, ptr2 );

    // Ta-da! This should output:
    ptr1->sayType();  // "DerivedClassOne"
    ptr2->sayType();  // "EmbarrassingDerivedClass. Wait.. I mean DerivedClassTwo!"
  }*/
}


