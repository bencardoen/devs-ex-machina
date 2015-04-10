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
#include "globallog.h"

namespace n_model {
class RootModel: public Model
{
private:
	std::vector<t_atomicmodelptr> m_components;
	bool m_directConnected;

public:
	RootModel();
	virtual ~RootModel();
	/**
	 * @precondition All atomic models have a unique name
	 */
	const std::vector<t_atomicmodelptr>& directConnect(t_coupledmodelptr&);
	void undoDirectConnect();
};
} /* namespace n_model */

#endif /* ROOTMODEL_H_ */
