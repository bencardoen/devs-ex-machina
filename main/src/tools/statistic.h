/*
 * This file is part of the DEVS Ex Machina project.
 * Copyright 2014 - 2015 University of Antwerp
 * https://www.uantwerpen.be/en/
 * Licensed under the EUPL V.1.1
 * A full copy of the license is in COPYING.txt, or can be found at
 * https://joinup.ec.europa.eu/community/eupl/og_page/eupl
 *      Author: Ben Cardoen, Stijn Manhaeve, Tim Tuijn
 */

#ifndef SRC_TOOLS_STATISTIC_H_
#define SRC_TOOLS_STATISTIC_H_

#include <string>
#include <iostream>
#include "tools/macros.h"

namespace n_tools {

/**
 * @brief Small object class for keeping track of a single statistic.
 *
 * A statistic contains a name, a unit and a value.
 * For generality, the value can have any type that supports
 * default initialization and the preincrement,
 * addition assignment and stream output operator.
 *
 * @tparam T The type of the data. Defaults to std::size_t
 * @tparam collect Boolean template argument. If true, data will be collected. Otherwise, no data is collected. Defaults to true.
 */

#ifdef USE_STAT
#define DEFAULTCOLLECT true
#else
#define DEFAULTCOLLECT false
#endif

template<typename T=std::size_t, bool collect=DEFAULTCOLLECT>
class Statistic {};

#undef DEFAULTCOLLECT

template<typename T>
class Statistic<T, true>
{
public:
	/**
	 * @brief type specifier of the contained datatype.
	 */
	typedef T t_datatype;
	/**
	 * @brief type specifier of this class.
	 */
	typedef Statistic<T, true> t_type;

	/**
	 * @brief Constructs a new Statistic object.
	 * @param name The name of the statistic.
	 * @param unit The unit of the statistic.
	 * @param start The starting value for the data.
	 */
	constexpr Statistic(const std::string& name, const std::string& unit, t_datatype start = t_datatype()):
		m_name(name),
		m_unit(unit),
		m_data(start)
	{
	}

	//needed for cereal. Don't use this one yourself. TODO
	//use the documented version instead!
	constexpr Statistic():
		m_name("__invalid_stat__"),
		m_unit("__invalid_unit__"),
		m_data(t_datatype())
	{
	}

	/**
	 * @brief pre increment operator
	 * Calls the pre increment operator of the data type.
	 */
	t_type& operator++()
	{
		++m_data;
		return *this;
	}
        
        const t_datatype& getData()const
        {
                return m_data;
        }

	/**
	 * @brief addition assignment operator
	 * Calls the addition assignment operator of the data type.
	 */
	t_type& operator+=(const t_datatype& value)
	{
		m_data += value;
		return *this;
	}

	/**
	 * @brief output stream operator.
	 */
	friend constexpr std::ostream& operator<<(std::ostream& out, const Statistic& value)
	{
	    out << value.m_data << "    " << value.m_name;
	    if(value.m_unit.length()) out << " (" << value.m_unit << ')';
		return (out << '\n');
	}
private:
	const std::string m_name;
	const std::string m_unit;
	t_datatype m_data;
	static const char delimiter = ',';
};

template<typename T>
class Statistic<T, false>
{
public:
	/**
	 * @brief type specifier of the contained datatype.
	 */
	typedef T t_datatype;
	/**
	 * @brief type specifier of this class.
	 */
	typedef Statistic<T, false> t_type;

	/**
	 * @brief Constructs a new Statistic object.
	 * @param name The name of the statistic.
	 * @param unit The unit of the statistic.
	 * @param start The starting value for the data.
	 */
	constexpr Statistic(const std::string&, const std::string&, t_datatype = t_datatype())
	{
	}

	//needed by cereal. Don't use this one yourself. TODO
	//use the documented version instead!
	constexpr Statistic()
	{
	}

	/**
	 * @brief pre increment operator
	 * Calls the pre increment operator of the data type.
	 */
	constexpr const t_type& operator++() const
	{
		return *this;
	}

	/**
	 * @brief addition assignment operator
	 * Calls the addition assignment operator of the data type.
	 */
	constexpr const t_type& operator+=(const t_datatype&) const
	{
		return *this;
	}

	/**
	 * @brief output stream operator.
	 */
	friend constexpr std::ostream& operator<<(std::ostream& out, const Statistic&)
	{
		return out;
	}
};

typedef Statistic<std::size_t> t_uintstat;
typedef Statistic<double> t_doublestat;

} /* namespace n_tools */

#endif /* SRC_TOOLS_STATISTIC_H_ */
