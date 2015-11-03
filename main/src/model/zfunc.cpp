/*
 * zfunc.cpp
 *
 *  Created on: Apr 1, 2015
 *      Author: Stijn Manhaeve - Devs Ex Machina
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
