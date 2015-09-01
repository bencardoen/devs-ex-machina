/*
 * rootModel.h
 *
 *  Created on: 9-mrt.-2015
 *      Author: Pieter, Tim
 */

#ifndef ROOTMODEL_H_
#define ROOTMODEL_H_

#include "model/coupledmodel.h"
#include "model/atomicmodel.h"
#include "tools/globallog.h"

namespace n_model {
class RootModel final//: public Model	//don't allow users to derive from this class
{
private:
	std::vector<t_atomicmodelptr> m_components;
	bool m_directConnected;

public:
	RootModel();
//	virtual ~RootModel();
	void setComponents(const t_coupledmodelptr& model);
	/**
	 * @precondition All atomic models have a unique name
	 */
	const std::vector<t_atomicmodelptr>& directConnect(const t_coupledmodelptr&);
	void undoDirectConnect();

	std::vector<t_atomicmodelptr> getComponents();
};
} /* namespace n_model */

#endif /* ROOTMODEL_H_ */
