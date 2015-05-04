/*
 * cell.cpp
 *
 *  Created on: May 1, 2015
 *      Author: Stijn Manhaeve - Devs Ex Machina
 */

#include <cellmodel.h>

namespace n_model {

n_model::CellAtomicModel::CellAtomicModel(std::string name, std::size_t priority):
	AtomicModel(name, priority)
{
}

const t_point& n_model::CellAtomicModel::getPoint() const
{
	return m_pos;
}

t_point n_model::CellAtomicModel::getPoint()
{
	return m_pos;
}

CellAtomicModel::CellAtomicModel(std::string name, t_point point, std::size_t priority):
	AtomicModel(name, priority),
	m_pos(point)
{
}

void n_model::CellAtomicModel::setPoint(t_point pt)
{
	m_pos = pt;
}

} /* namespace n_model */
