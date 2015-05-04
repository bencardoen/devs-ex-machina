/*
 * FireCell.h
 *
 *  Created on: May 2, 2015
 *      Author: Stijn Manhaeve - Devs Ex Machina
 */

#ifndef SRC_EXAMPLES_FORESTFIRE_FIRECELLSTATE_H_
#define SRC_EXAMPLES_FORESTFIRE_FIRECELLSTATE_H_

#include <cellmodel.h>
#include "constants.h"
#include <array>

namespace n_examples {

class FireCell;

class FireCellState: public n_model::State
{
	friend FireCell;
private:
	double m_temperature;
	std::size_t m_igniteTime;
	FirePhase m_phase;
	std::array<double, 4> m_surroundingTemp;
	double m_oldTemp;

	double getSurroundingTemp() const;
public:
	FireCellState(double temp = T_AMBIENT);

	std::string toString() override;

	std::string toCell() override;

	//TODO @Pieter serialize FireCell
};

typedef std::shared_ptr<FireCellState> t_firecellstateptr;

} /* namespace n_examples */

#endif /* SRC_EXAMPLES_FORESTFIRE_FIRECELLSTATE_H_ */
