/*
 * This file is part of the DEVS Ex Machina project.
 * Copyright 2014 - 2015 University of Antwerp
 * https://www.uantwerpen.be/en/
 * Licensed under the EUPL V.1.1
 * A full copy of the license is in COPYING.txt, or can be found at
 * https://joinup.ec.europa.eu/community/eupl/og_page/eupl
 *      Author: Stijn Manhaeve, Ben Cardoen, Tim Tuijn, Pieter Lauwers
 */

#ifndef SRC_MODEL_ZFUNC_H_
#define SRC_MODEL_ZFUNC_H_

#include "network/message.h"
#include <memory>

namespace n_model {

/**
 * @brief Z function base functor class
 */
class ZFunc
{
public:
	virtual ~ZFunc();
	virtual n_network::t_msgptr operator()(const n_network::t_msgptr&);
};

typedef std::shared_ptr<ZFunc> t_zfunc;

/**
 * @brief Z function combinator functor class
 */
class ZFuncCombo final: public ZFunc{
private:
	t_zfunc m_left;
	t_zfunc m_right;

public:
	ZFuncCombo(const t_zfunc& left, const t_zfunc& right);
	virtual ~ZFuncCombo();

	virtual n_network::t_msgptr operator()(const n_network::t_msgptr&);
};


} /* namespace n_model */

#endif /* SRC_MODEL_ZFUNC_H_ */
