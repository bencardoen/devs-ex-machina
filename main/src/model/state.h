/*
 * state.h
 *
 *  Created on: 9-mrt.-2015
 *      Author: Pieter, Tim, Stijn
 */

#ifndef STATE_H_
#define STATE_H_

#include "network/timestamp.h"
#include "tools/globallog.h"
#include "tools/objectfactory.h"
#include <assert.h>
#include "serialization/archive.h"
#include "cereal/types/polymorphic.hpp"

namespace n_model {

using n_network::t_timestamp;

class State;

typedef std::shared_ptr<State> t_stateptr;

/**
 * @brief Keeps track of the current state of a model.
 */
class State
{
public:
	t_timestamp m_timeLast;
	t_timestamp m_timeNext;

	State() = default;

	/**
	 * @brief Converts the state as a regular string
	 */
	virtual std::string toString() const
	{
		LOG_ERROR("STATE: Not implemented: 'std::string n_model::State::toString()'");
		return "";
	}

	/**
	 * @brief Converts the state to an xml string
	 * @see XMLTracer
	 */
	virtual std::string toXML() const
	{
		LOG_ERROR("STATE: Not implemented: 'std::string n_model::State::toXML()'");
		return "";
	}

	/**
	 * @brief Converts the state to a JSON string
	 * @see JSONTracer
	 */
	virtual std::string toJSON() const
	{
		LOG_ERROR("STATE: Not implemented: 'std::string n_model::State::toJSON()'");
		return "";
	}

	/**
	 * @brief Converts the state to a Cell string
	 * @see CELLTracer
	 */
	virtual std::string toCell() const
	{
		LOG_ERROR("STATE: Not implemented: 'std::string n_model::State::toCell()'");
		return "";
	}

	virtual ~State()
	{
	}

	/**
	 * @brief Creates a copy of the string
	 */
	virtual t_stateptr copyState() const
	{
		LOG_ERROR("STATE: Not implemented: 'std::string n_model::State::copyState()'");
		assert(false && "State::copyState not reimplemented in derived class.");
		return nullptr;
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
	static void load_and_construct(n_serialization::t_iarchive& archive, cereal::construct<State>& construct);
};

template<typename T>
class State__impl: public State
{
public:
	typedef T t_type;

	t_type m_value;

	State__impl(const T value = T()): m_value(value)
	{}

	t_stateptr copyState() const override
	{
		return n_tools::createObject<State__impl<T>>(*this);
	}


	/**
	 * @brief Converts the state to an xml string
	 * @see XMLTracer
	 */
	virtual std::string toString() const override
	{
		return m_value.toString();
	}

	/**
	 * @brief Converts the state to an xml string
	 * @see XMLTracer
	 */
	virtual std::string toXML() const override
	{
		return m_value.toXML();
	}

	/**
	 * @brief Converts the state to a JSON string
	 * @see JSONTracer
	 */
	virtual std::string toJSON() const override
	{
		return m_value.toJSON();
	}

	/**
	 * @brief Converts the state to a Cell string
	 * @see CELLTracer
	 */
	virtual std::string toCell() const override
	{
		return m_value.toCell();
	}
};

template<>
class State__impl<void>: public State
{
public:
	typedef void t_type;

	State__impl()
	{}

	t_stateptr copyState() const override
	{
		return n_tools::createObject<State__impl<void>>(*this);
	}


	/**
	 * @brief Converts the state to an xml string
	 * @see XMLTracer
	 */
	virtual std::string toString() const override
	{
		return "";
	}

	/**
	 * @brief Converts the state to an xml string
	 * @see XMLTracer
	 */
	virtual std::string toXML() const override
	{
		return "";
	}

	/**
	 * @brief Converts the state to a JSON string
	 * @see JSONTracer
	 */
	virtual std::string toJSON() const override
	{
		return "";
	}

	/**
	 * @brief Converts the state to a Cell string
	 * @see CELLTracer
	 */
	virtual std::string toCell() const override
	{
		return "";
	}
};
}

#endif /* STATE_H_ */
