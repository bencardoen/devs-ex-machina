/*
 * zfunc.h
 *
 *  Created on: Apr 1, 2015
 *      Author: Stijn Manhaeve - Devs Ex Machina
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
