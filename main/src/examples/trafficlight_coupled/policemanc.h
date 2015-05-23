/*
 * policemanc.h
 *
 *  Created on: Mar 28, 2015
 *      Author: tim
 */

#ifndef MAIN_SRC_EXAMPLES_TRAFFICLIGHT_COUPLED_POLICEMANC_H_
#define MAIN_SRC_EXAMPLES_TRAFFICLIGHT_COUPLED_POLICEMANC_H_

#include "atomicmodel.h"
#include "state.h"
#include <assert.h>

namespace n_examples_coupled {

using n_model::State;
using n_model::t_stateptr;
using n_model::AtomicModel;
using n_network::t_msgptr;
using n_network::t_timestamp;
using n_model::t_atomicmodelptr;

class PolicemanMode: public State
{
public:
	PolicemanMode(std::string state);
	std::string toXML() override;
	std::string toJSON() override;
	std::string toCell() override;
	~PolicemanMode()
	{
	}

	/**
	 * Serialize this object to the given archive
	 *
	 * @param archive A container for the desired output stream
	 */
	void serialize(n_serialization::t_oarchive& archive);

	/**
	 * Unserialize this object to the given archive
	 *
	 * @param archive A container for the desired input stream
	 */
	void serialize(n_serialization::t_iarchive& archive);

	/**
	 * Helper function for unserializing smart pointers to an object of this class.
	 *
	 * @param archive A container for the desired input stream
	 * @param construct A helper struct for constructing the original object
	 */
	static void load_and_construct(n_serialization::t_iarchive& archive, cereal::construct<PolicemanMode>& construct);
};

class Policeman: public AtomicModel
{
public:
	Policeman() = delete;
	Policeman(std::string, std::size_t priority = 0);
	~Policeman()
	{
	}

	void extTransition(const std::vector<n_network::t_msgptr> & message) override;
	void intTransition() override;
	t_timestamp timeAdvance() const override;
	std::vector<n_network::t_msgptr> output() const override;
	t_timestamp lookAhead() const override;

	/*
	 * The following function has been created to easily
	 * create states using a string
	 */
	using AtomicModel::setState;
	t_stateptr setState(std::string);

	/**
	 * Serialize this object to the given archive
	 *
	 * @param archive A container for the desired output stream
	 */
	void serialize(n_serialization::t_oarchive& archive);

	/**
	 * Unserialize this object to the given archive
	 *
	 * @param archive A container for the desired input stream
	 */
	void serialize(n_serialization::t_iarchive& archive);

	/**
	 * Helper function for unserializing smart pointers to an object of this class.
	 *
	 * @param archive A container for the desired input stream
	 * @param construct A helper struct for constructing the original object
	 */
	static void load_and_construct(n_serialization::t_iarchive& archive, cereal::construct<Policeman>& construct);
};

}

CEREAL_REGISTER_TYPE(n_examples_coupled::Policeman)
CEREAL_REGISTER_TYPE(n_examples_coupled::PolicemanMode)

#endif /* MAIN_SRC_EXAMPLES_TRAFFICLIGHT_COUPLED_POLICEMANC_H_ */
