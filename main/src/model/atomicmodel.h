/*
 * atomicModel.h
 *
 *  Created on: 9-mrt.-2015
 *      Author: Pieter, Tim
 */

#ifndef ATOMICMODEL_H_
#define ATOMICMODEL_H_

#include "model.h"
#include "message.h"
#include <map>

namespace n_model {
class AtomicModel
{
private:
	t_timestamp m_elapsed;
	t_timestamp m_lastRead;

public:
	State extTransition(const n_network::t_msgptr & message);
	State intTransition();
	State confTransition(const n_network::t_msgptr & message);
	t_timestamp timeAdvance();
	std::map<t_portptr, n_network::t_msgptr> output();
	void setGVT(t_timestamp gvt);
	void revert(t_timestamp time);
};
}

#endif /* ATOMICMODEL_H_ */
