/*
 * statistic.h
 *
 *  Created on: Aug 12, 2015
 *      Author: Stijn Manhaeve @ DEVS Ex Machina
 */

#ifndef SRC_TOOLS_STATISTIC_H_
#define SRC_TOOLS_STATISTIC_H_

//TODO remove USESTAT definition when testing is done!
//#ifndef USESTAT
//#define USESTAT
//#endif

#include <string>
#include <iostream>
#include "tools/globallog.h"

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
 */
template<typename T = std::size_t>
class Statistic
{
public:
	/**
	 * @brief type specifier of the contained datatype.
	 */
	typedef T t_datatype;
	/**
	 * @brief type specifier of this class.
	 */
	typedef Statistic<T> t_type;

	/**
	 * @brief Constructs a new Statistic object.
	 * @param name The name of the statistic.
	 * @param unit The unit of the statistic.
	 * @param start The starting value for the data.
	 */
	Statistic(std::string name, std::string unit, t_datatype start = t_datatype()):
		m_name(name),
		m_unit(unit),
		m_data(start)
	{
	}

	//needed by cereal. Don't use this one yourself.
	//use the documented version instead!
	Statistic():
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
	friend std::ostream& operator<<(std::ostream& out, const Statistic& value)
	{
		out << value.m_name << Statistic::delimiter << value.m_data << Statistic::delimiter << value.m_unit;
		return out;
	}
private:
	const std::string m_name;
	const std::string m_unit;
	t_datatype m_data;
	static const char delimiter = ',';
};

typedef Statistic<std::size_t> t_intstat;
typedef Statistic<double> t_doublestat;

} /* namespace n_tools */

#endif /* SRC_TOOLS_STATISTIC_H_ */
