/*
 * trafficsystemc.cpp
 *
 *  Created on: Mar 29, 2015
 *      Author: tim
 */

#include "trafficsystemc.h"

namespace n_examples_coupled {


TrafficSystem::TrafficSystem(std::string name) : CoupledModel(name) {
	t_atomicmodelptr policeman = n_tools::createObject<Policeman>("policeman");
	t_atomicmodelptr trafficlight = n_tools::createObject<TrafficLight>("trafficLight");

	this->addSubModel(policeman);
	this->addSubModel(trafficlight);

	this->connectPorts(policeman->getPort("OUT"), trafficlight->getPort("INTERRUPT"));

}

void TrafficSystem::serialize(n_serialization::t_oarchive& archive)
{
	LOG_INFO("SERIALIZATION: Saving Traffic System '", getName(), "' with timeNext = ", m_timeNext);
	archive(cereal::virtual_base_class<CoupledModel>( this ));
}

void TrafficSystem::serialize(n_serialization::t_iarchive& archive)
{
	archive(cereal::virtual_base_class<CoupledModel>( this ));
	LOG_INFO("SERIALIZATION: Loaded Traffic System '", getName(), "' with timeNext = ", m_timeNext);
}

void TrafficSystem::load_and_construct(n_serialization::t_iarchive& archive, cereal::construct<TrafficSystem>& construct)
{
	construct("");
	construct->serialize(archive);
}

}

