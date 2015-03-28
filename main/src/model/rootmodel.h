/*
 * rootModel.h
 *
 *  Created on: 9-mrt.-2015
 *      Author: Pieter, Tim
 */

#ifndef ROOTMODEL_H_
#define ROOTMODEL_H_

#include "coupledmodel.h"
#include "atomicmodel.h"

namespace n_model {
class RootModel: public Model
{
private:
	std::vector<t_atomicmodelptr> m_components;
	bool m_directConnected;

public:
	std::vector<t_atomicmodelptr> directConnect(const t_coupledmodelptr&);
	void undoDirectConnect();
};
}

#endif /* ROOTMODEL_H_ */
