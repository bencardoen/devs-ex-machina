/*
 * zfunc.cpp
 *
 *  Created on: Apr 1, 2015
 *      Author: Stijn Manhaeve - Devs Ex Machina
 */

#include <zfunc.h>

namespace n_model {

ZFunc::~ZFunc()
{
}

n_network::t_msgptr ZFunc::operator()(const n_network::t_msgptr& msg)
{
	return msg;
}

void ZFunc::serialize(n_serialisation::t_oarchive& archive)
{
}

void ZFunc::serialize(n_serialisation::t_iarchive& archive)
{
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
