/*
 * This file is part of the DEVS Ex Machina project.
 * Copyright 2014 - 2015 University of Antwerp 
 * https://www.uantwerpen.be/en/
 * Licensed under the EUPL V.1.1
 * A full copy of the license is in COPYING.txt, or can be found at 
 * https://joinup.ec.europa.eu/community/eupl/og_page/eupl 
 *      Author: Stijn Manhaeve
 */

#include "model/cellmodel.h"

namespace n_model {

CellAtomicModel_impl::CellAtomicModel_impl(std::string name, std::size_t priority):
	AtomicModel_impl(name, priority)
{
}

CellAtomicModel_impl::CellAtomicModel_impl(std::string name, t_point point, std::size_t priority):
	AtomicModel_impl(name, priority),
	m_pos(point)
{
}

CellAtomicModel_impl::CellAtomicModel_impl(std::string name, int core, std::size_t priority):
	AtomicModel_impl(name, core, priority)
{
}

CellAtomicModel_impl::CellAtomicModel_impl(std::string name, t_point point, int core, std::size_t priority):
	AtomicModel_impl(name, core, priority),
	m_pos(point)
{
}

void n_model::CellAtomicModel_impl::setPoint(t_point pt)
{
	m_pos = pt;
}

const t_point& n_model::CellAtomicModel_impl::getPoint() const
{
	return m_pos;
}

t_point n_model::CellAtomicModel_impl::getPoint()
{
	return m_pos;
}

} /* namespace n_model */
