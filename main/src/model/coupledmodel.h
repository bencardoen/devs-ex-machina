/*
 * coupledModel.h
 *
 *  Created on: 9-mrt.-2015
 *      Author: Pieter, Tim
 */

#ifndef COUPLEDMODEL_H_
#define COUPLEDMODEL_H_

#include "model.h"

namespace n_model {
class CoupledModel: public Model
{
private:
	std::vector<t_modelptr> m_components;

public:
	void addSubModel(const t_modelptr& model);
	void connectPorts(const t_portptr& p1, const t_portptr& p2, t_zfunc zFunction);
	std::vector<t_modelptr> getComponents() const;
};

typedef std::shared_ptr<CoupledModel> t_coupledmodelptr;
}

#endif /* COUPLEDMODEL_H_ */
