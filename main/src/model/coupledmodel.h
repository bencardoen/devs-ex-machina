/*
 * coupledModel.h
 *
 *  Created on: 9-mrt.-2015
 *      Author: Pieter, Tim
 */

#ifndef COUPLEDMODEL_H_
#define COUPLEDMODEL_H_

#include "atomicmodel.h"

namespace n_model {
class CoupledModel
{
private:
	std::vector<Model> m_components;

public:
	void flatten();
	void unflatten();
	AtomicModel select(std::vector<AtomicModel> immChildren);
	void addSubModel(AtomicModel model);
	void removeSubModel(AtomicModel model);
	void connectPorts(Port p1, Port p2, std::function<void> zFunction);
	void disconnectPorts(Port p1, Port p2);
};
}

#endif /* COUPLEDMODEL_H_ */
