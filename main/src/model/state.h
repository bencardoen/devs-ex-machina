/*
 * This file is part of the DEVS Ex Machina project.
 * Copyright 2014 - 2015 University of Antwerp 
 * https://www.uantwerpen.be/en/
 * Licensed under the EUPL V.1.1
 * A full copy of the license is in COPYING.txt, or can be found at 
 * https://joinup.ec.europa.eu/community/eupl/og_page/eupl 
 *      Author: Stijn Manhaeve, Ben Cardoen, Tim Tuijn, Pieter Lauwers
 */
#ifndef STATE_H_
#define STATE_H_

#include "network/timestamp.h"
#include "tools/globallog.h"
#include "tools/objectfactory.h"
#include "tools/macros.h"
#include "tools/stringtools.h"
#include <assert.h>
#include <typeinfo>

// representation of states

// base case. The default implementation will generate an error message
#define STATE_REPR_STRUCT(__name) \
template<typename T> struct __name { \
	static std::string exec(const T&) { \
		return std::string("No " STRINGIFY(__name) "::exec overload for type ") + typeid(T).name(); \
	} \
}

// Create these messages for names ToString, ToXML, ToJSON and ToCell
STATE_REPR_STRUCT(ToString);
STATE_REPR_STRUCT(ToXML);
STATE_REPR_STRUCT(ToJSON);
STATE_REPR_STRUCT(ToCell);

// Provide default implementations for numeric types, but only if not using cygwin
#ifndef __CYGWIN__
#define STATE_REPR_ARITHMETIC(__type, __name) \
template<> struct __name<__type> { \
	static std::string exec(const __type& val) { \
		return std::to_string(val); \
	} \
}
#else //ifndef __CYGWIN__
#define STATE_REPR_ARITHMETIC(__type, __name) \
template<> struct __name<__type> { \
    static std::string exec(const __type& val) { \
        return n_tools::toString<__type>(val); \
    } \
}
#endif //ifndef __CYGWIN__
#define STATE_REPR_ARITHMETIC_GROUP(__name) \
STATE_REPR_ARITHMETIC(int, __name); \
STATE_REPR_ARITHMETIC(unsigned, __name); \
STATE_REPR_ARITHMETIC(long, __name); \
STATE_REPR_ARITHMETIC(unsigned long, __name); \
STATE_REPR_ARITHMETIC(long long, __name); \
STATE_REPR_ARITHMETIC(unsigned long long, __name); \
STATE_REPR_ARITHMETIC(float, __name); \
STATE_REPR_ARITHMETIC(double, __name); \
STATE_REPR_ARITHMETIC(long double, __name)

STATE_REPR_ARITHMETIC_GROUP(ToString);
STATE_REPR_ARITHMETIC_GROUP(ToXML);
STATE_REPR_ARITHMETIC_GROUP(ToJSON);
STATE_REPR_ARITHMETIC_GROUP(ToCell);

template<> struct ToString<bool> {
	static std::string exec(const bool& val) {
		return val? "1":"0";
	}
};
template<> struct ToXML<bool> {
	static std::string exec(const bool& val) {
		return val? "true":"false";
	}
};
template<> struct ToJSON<bool> {
	static std::string exec(const bool& val) {
		return val? "true":"false";
	}
};
template<> struct ToCell<bool> {
	static std::string exec(const bool& val) {
		return val? "1":"0";
	}
};
template<> struct ToString<std::string> {
	static std::string exec(const std::string& val) {
		return val;
	}
};
template<> struct ToXML<std::string> {
	static std::string exec(const std::string& val) {
		return val;
	}
};
template<> struct ToJSON<std::string> {
	static std::string exec(const std::string& val) {
		return "\"" + val + "\"";
	}
};
template<> struct ToCell<std::string> {
	static std::string exec(const std::string& val) {
		return val;
	}
};

#undef STATE_REPR_STRUCT
#undef STATE_REPR_ARITHMETIC
#undef STATE_REPR_ARITHMETIC_GROUP

namespace n_model {

using n_network::t_timestamp;

class State;
typedef State* t_stateptr;

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
		LOG_ERROR("STATE: Not implemented: 't_stateptr n_model::State::copyState()'");
		assert(false && "State::copyState not reimplemented in derived class.");
		return nullptr;
	}
	virtual t_stateptr copyPooledState() const
	{
	        LOG_ERROR("STATE: Not implemented: 't_stateptr n_model::State::copyPooledState()'");
	        assert(false && "State::copyPooledState not reimplemented in derived class.");
	        return nullptr;
	}

	const t_timestamp& getTimeLast() const
	{
		return m_timeLast;
	}

	const t_timestamp& getTimeNext() const
	{
		return m_timeNext;
	}

	void setTimeLast(const t_timestamp& timeLast)
	{
		m_timeLast = timeLast;
	}

	void setTimeNext(const t_timestamp& timeNext)
	{
		m_timeNext = timeNext;
	}

	/**
     * Destroys this object by running the destructor and removing it from the heap.
	 */
    virtual void releaseMe()
    {
            n_tools::destroyPooledObject<typename std::remove_pointer<decltype(this)>::type>(this);
    }

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
	        return n_tools::createRawObject<State__impl<T>>(*this);
	}

    t_stateptr copyPooledState() const override
    {
            return n_tools::createPooledObject<State__impl<T>>(*this);
    }


	/**
	 * @brief Converts the state to an xml string
	 * @see XMLTracer
	 */
	virtual std::string toString() const override
	{
		return ToString<t_type>::exec(m_value);
	}

	/**
	 * @brief Converts the state to an xml string
	 * @see XMLTracer
	 */
	virtual std::string toXML() const override
	{
		return ToXML<t_type>::exec(m_value);
	}

	/**
	 * @brief Converts the state to a JSON string
	 * @see JSONTracer
	 */
	virtual std::string toJSON() const override
	{
		return ToJSON<t_type>::exec(m_value);
	}

	/**
	 * @brief Converts the state to a Cell string
	 * @see CELLTracer
	 */
	virtual std::string toCell() const override
	{
		return ToCell<t_type>::exec(m_value);
	}

	/**
	 * Destroys this object by running the destructor and removing it from the heap.
	 * Note: only call this method if the object was created with an object pool.
	 */
    virtual void releaseMe()override
    {
            n_tools::destroyPooledObject<typename std::remove_pointer<decltype(this)>::type>(this);
    }
};

template<>
class State__impl<void> final: public State
{
public:
	typedef void t_type;

	State__impl()
	{}

	t_stateptr copyState() const override
	{
		return n_tools::createRawObject<State__impl<void>>(*this);
	}

    t_stateptr copyPooledState() const override
    {
        return n_tools::createPooledObject<State__impl<void>>(*this);
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

	/**
     * Destroys this object by running the destructor and removing it from the heap.
	 */
    virtual void releaseMe()override
    {
            n_tools::destroyPooledObject<typename std::remove_pointer<decltype(this)>::type>(this);
    }
};
}

#endif /* STATE_H_ */
