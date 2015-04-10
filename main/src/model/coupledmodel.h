/*
 * coupledModel.h
 *
 *  Created on: 9-mrt.-2015
 *      Author: Pieter, Tim
 */

#ifndef COUPLEDMODEL_H_
#define COUPLEDMODEL_H_

#include "model.h"
#include "objectfactory.h"

namespace n_model {
class CoupledModel: public Model
{
protected:
	std::vector<t_modelptr> m_components;

public:
	CoupledModel() = delete;

	/**
	 * Constructor for CoupledModel
	 *
	 * @param name The name of the coupled model
	 */
	CoupledModel(std::string name);

	/**
	 * Virtual destructor for coupled model
	 */
	virtual ~CoupledModel()
	{
	}

	/**
	 * Adds a submodel to this coupled model
	 *
	 * @param model The submodel that is to be added
	 */
	void addSubModel(const t_modelptr& model);

	/**
	 * Resets the parent pointers of this model and all its children (depth-first)
	 */
	virtual void resetParents();

	/**
	 * Connects the given ports with eachother (with a zFunction)
	 * The zFunction will not change the message by default
	 *
	 * @param p1 pointer to output port number
	 * @param p2 pointer to input port number
	 * @param zFunction the zFunction that has to be executed on the message before transmitting it
	 */
	void connectPorts(const t_portptr& p1, const t_portptr& p2, t_zfunc zFunction = n_tools::createObject<ZFunc>());

	/**
	 * Get all the components (submodels)
	 *
	 * @return a vector with pointers to these models
	 */
	std::vector<t_modelptr> getComponents() const;
};

typedef std::shared_ptr<CoupledModel> t_coupledmodelptr;
}

#endif /* COUPLEDMODEL_H_ */
