/*
 * myclasses.h
 *
 *  Created on: May 19, 2015
 *      Author: pieter
 */

#ifndef SRC_TEST_BASICCEREALTESTCLASSES_H_
#define SRC_TEST_BASICCEREALTESTCLASSES_H_

// Include the polymorphic serialization and registration mechanisms
#include "cereal/types/polymorphic.hpp"

// For LoadAndConstruct
/*#include "cereal/access.hpp"*/

 // A pure virtual base class
struct BaseClass
{
  virtual void sayType() = 0;
  virtual ~BaseClass() {}
};

// A class derived from BaseClass
struct DerivedClassOne : public BaseClass
{
	void sayType();

	int x;

	template<class Archive>
	void save(Archive & archive) const;

	template<class Archive>
	void load(Archive & archive);

	/*
	template<class Archive>
	static void load_and_construct(Archive& archive, cereal::construct<DerivedClassOne>& construct);
	*/
};

// Another class derived from BaseClass
struct EmbarrassingDerivedClass : public BaseClass
{
	void sayType();

	float y;

	template<class Archive>
	void save(Archive & archive) const;

	template<class Archive>
	void load(Archive & archive);

	/*
	template<class Archive>
	static void load_and_construct(Archive& archive, cereal::construct<EmbarrassingDerivedClass>& construct);
	*/
};

// Include any archives you plan on using with your type before you register it
// Note that this could be done in any other location so long as it was prior
// to this file being included
#include "cereal/archives/binary.hpp"
#include "cereal/archives/xml.hpp"
#include "cereal/archives/json.hpp"

// Register DerivedClassOne
CEREAL_REGISTER_TYPE(DerivedClassOne)

// Register EmbarassingDerivedClass with a less embarrasing name
CEREAL_REGISTER_TYPE_WITH_NAME(EmbarrassingDerivedClass, "DerivedClassTwo")

// Note that there is no need to register the base class, only derived classes



#endif /* SRC_TEST_BASICCEREALTESTCLASSES_H_ */
