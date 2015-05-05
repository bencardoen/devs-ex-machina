/*
 * FireCell.cpp
 *
 *  Created on: May 2, 2015
 *      Author: Stijn Manhaeve - Devs Ex Machina
 */

#include <examples/forestfire/firecellstate.h>
#include <limits>
#include <sstream>
#include <iomanip>

namespace n_examples {

} /* namespace n_examples */

n_examples::FireCellState::FireCellState(double temp):
	m_temperature(temp),
	m_igniteTime(std::numeric_limits<std::size_t>::max()),
	m_phase(FirePhase::INACTIVE),
	m_surroundingTemp({{T_AMBIENT, T_AMBIENT, T_AMBIENT, T_AMBIENT}}),
	m_oldTemp(temp)
{
}

std::string n_examples::FireCellState::toString()
{
	std::stringstream ssr;
	ssr << to_string(m_phase) << " (T: " << m_temperature << ')';
	return ssr.str();
}

std::string n_examples::FireCellState::toCell()
{
#ifndef __CYGWIN__
	return std::to_string(m_temperature);
#else
	std::stringstream ssr;
	ssr.precision(6);
	ssr << std::fixed << m_temperature;
	return ssr.str();
#endif
}

double n_examples::FireCellState::getSurroundingTemp() const
{
	return m_surroundingTemp[0] + m_surroundingTemp[1] + m_surroundingTemp[2] + m_surroundingTemp[3];
}

void n_examples::FireCellState::setTemp(double t)
{
	m_temperature = t;
}
