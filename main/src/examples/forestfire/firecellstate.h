/*
 * This file is part of the DEVS Ex Machina project.
 * Copyright 2014 - 2015 University of Antwerp 
 * https://www.uantwerpen.be/en/
 * Licensed under the EUPL V.1.1
 * A full copy of the license is in COPYING.txt, or can be found at 
 * https://joinup.ec.europa.eu/community/eupl/og_page/eupl 
 *      Author: Stijn Manhaeve
 */

#ifndef SRC_EXAMPLES_FORESTFIRE_FIRECELLSTATE_H_
#define SRC_EXAMPLES_FORESTFIRE_FIRECELLSTATE_H_

#include "model/cellmodel.h"
#include "examples/forestfire/constants.h"
#include <array>
#include <sstream>
#include <iomanip>

namespace n_examples {

struct FireCellState
{
	double m_temperature;
	std::size_t m_igniteTime;
	FirePhase m_phase;
	std::array<double, 4> m_surroundingTemp;
	double m_oldTemp;

	constexpr FireCellState(double temp = T_AMBIENT):
		m_temperature(temp),
		m_igniteTime(std::numeric_limits<std::size_t>::max()),
		m_phase(FirePhase::INACTIVE),
		m_surroundingTemp({{T_AMBIENT, T_AMBIENT, T_AMBIENT, T_AMBIENT}}),
		m_oldTemp(temp)
	{
	}

	constexpr double getSurroundingTemp() const
	{
		return m_surroundingTemp[0] + m_surroundingTemp[1] + m_surroundingTemp[2] + m_surroundingTemp[3];
	}
};

} /* namespace n_examples */


template<>
struct ToString<n_examples::FireCellState>
{
	static std::string exec(const n_examples::FireCellState& s){
		std::stringstream ssr;
		ssr << to_string(s.m_phase) << " (T: " << s.m_temperature << ')';
		return ssr.str();
	}
};
template<>
struct ToCell<n_examples::FireCellState>
{
	static std::string exec(const n_examples::FireCellState& s){
#ifndef __CYGWIN__
		return std::to_string(s.m_temperature);
#else
		std::stringstream ssr;
		ssr.precision(6);
		ssr << std::fixed << m_temperature;
		return ssr.str();
#endif
	}
};

#endif /* SRC_EXAMPLES_FORESTFIRE_FIRECELLSTATE_H_ */
