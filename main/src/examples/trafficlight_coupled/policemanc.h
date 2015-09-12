/*
 * policemanc.h
 *
 *  Created on: Mar 28, 2015
 *      Author: tim
 */

#ifndef MAIN_SRC_EXAMPLES_TRAFFICLIGHT_COUPLED_POLICEMANC_H_
#define MAIN_SRC_EXAMPLES_TRAFFICLIGHT_COUPLED_POLICEMANC_H_

#include "model/atomicmodel.h"
#include "model/state.h"
#include <assert.h>

namespace n_examples_coupled {

using n_model::State;
using n_model::t_stateptr;
using n_model::AtomicModel;
using n_network::t_msgptr;
using n_network::t_timestamp;
using n_model::t_atomicmodelptr;


class PolicemanMode
{
public:
	std::string m_value;
	PolicemanMode(std::string state);
};


} /* namespace n_examples */

template<>
struct ToString<n_examples_coupled::PolicemanMode>
{
	static std::string exec(const n_examples_coupled::PolicemanMode& s){
		return s.m_value;
	}
};
template<>
struct ToXML<n_examples_coupled::PolicemanMode>
{
	static std::string exec(const n_examples_coupled::PolicemanMode& s){
		return "<policeman activity =\"" + s.m_value + "\"/>";
	}
};
template<>
struct ToJSON<n_examples_coupled::PolicemanMode>
{
	static std::string exec(const n_examples_coupled::PolicemanMode& s){
		return "{ \"activity\": \"" + s.m_value + "\" }";
	}
};
template<>
struct ToCell<n_examples_coupled::PolicemanMode>
{
	static std::string exec(const n_examples_coupled::PolicemanMode& s){
		return "";
	}
};

namespace n_examples_coupled {

class Policeman: public AtomicModel<PolicemanMode>
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
	void output(std::vector<n_network::t_msgptr>& msgs) const override;
	t_timestamp lookAhead() const override;

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

#endif /* MAIN_SRC_EXAMPLES_TRAFFICLIGHT_COUPLED_POLICEMANC_H_ */
