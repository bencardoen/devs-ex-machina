/*
 * myclasses.cpp
 *
 *  Created on: May 19, 2015
 *      Author: pieter
 */

#include "test/basiccerealtestclasses.h"
#include <iostream>

void DerivedClassOne::sayType()
{
  std::cout << "DerivedClassOne" << std::endl;
}

void EmbarrassingDerivedClass::sayType()
{
  std::cout << "EmbarrassingDerivedClass. Wait.. I mean DerivedClassTwo!" << std::endl;
}


template<class Archive>
    void DerivedClassOne::save(Archive & archive) const
{
	archive(x);
}

template<class Archive>
void DerivedClassOne::load(Archive & archive)
{
	archive(x);
}

template<class Archive>
	void EmbarrassingDerivedClass::save(Archive & archive) const
{
	archive(y);
}

template<class Archive>
void EmbarrassingDerivedClass::load(Archive & archive)
{
	archive(y);
}

