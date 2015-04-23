/*
 * state.h
 *
 *  Created on: 9-mrt.-2015
 *      Author: Pieter, Tim
 */

#ifndef STATE_H_
#define STATE_H_

#include "timestamp.h"
#include "archive.h"
#include "tools/globallog.h"
#include <assert.h>
#include "cereal/types/polymorphic.hpp"

namespace n_model {

using n_network::t_timestamp;

class State
{
public:
	t_timestamp m_timeLast;
	t_timestamp m_timeNext;

	std::string m_state;

	State(std::string state)
	{
		m_state = state;
	}

	void setNull();

	virtual std::string toString()
	{
		return m_state;
	}

	virtual std::string toXML()
	{
		LOG_ERROR("STATE: Not implemented: 'std::string n_model::State::toXML()'");
		return "";
	}

	virtual std::string toJSON()
	{
		LOG_ERROR("STATE: Not implemented: 'std::string n_model::State::toJSON()'");
		return "";
	}

	virtual std::string toCell()
	{
		LOG_ERROR("STATE: Not implemented: 'std::string n_model::State::toCell()'");
		return "";
	}

	virtual ~State()
	{
	}

	friend bool operator==(const State& lhs, const std::string rhs);
	friend bool operator==(const std::string lhs, const State& rhs);

	/**
	 * Serialize this object to the given archive
	 *
	 * @param archive A container for the desired output stream
	 */
	void serialize(n_serialisation::t_oarchive& archive);

	/**
	 * Unserialize this object to the given archive
	 *
	 * @param archive A container for the desired input stream
	 */
	void serialize(n_serialisation::t_iarchive& archive);

	/**
	 * Helper function for unserializing smart pointers to an object of this class.
	 *
	 * @param archive A container for the desired input stream
	 * @param construct A helper struct for constructing the original object
	 */
	static void load_and_construct(n_serialisation::t_iarchive& archive, cereal::construct<State>& construct);
};

typedef std::shared_ptr<State> t_stateptr;
}

#endif /* STATE_H_ */
