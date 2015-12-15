/*
 * This file is part of the DEVS Ex Machina project.
 * Copyright 2014 - 2015 University of Antwerp
 * https://www.uantwerpen.be/en/
 * Licensed under the EUPL V.1.1
 * A full copy of the license is in COPYING.txt, or can be found at
 * https://joinup.ec.europa.eu/community/eupl/og_page/eupl
 *      Author: Stijn Manhaeve, Tim Tuijn, Pieter Lauwers
 */

#include "model/zfunc.h"

namespace n_model {

ZFunc::~ZFunc()
{
}

n_network::t_msgptr ZFunc::operator()(const n_network::t_msgptr& msg)
{
	return msg;
}

ZFuncCombo::ZFuncCombo(const t_zfunc& left, const t_zfunc& right):
	m_left(left), m_right(right)
{
}

ZFuncCombo::~ZFuncCombo()
{
}

n_network::t_msgptr ZFuncCombo::operator ()(const n_network::t_msgptr& msg)
{
	return (*m_right)((*m_left)(msg));
}

}
