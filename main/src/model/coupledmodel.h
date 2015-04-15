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
private:
	using Model::m_control;	//change access to private

protected:
	std::vector<t_modelptr> m_components;
	void setController(n_control::Controller* newControl);

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
	 * Removes a submodel from this coupled model
	 *
	 * @param model The submodel that is to be removed
	 * @note If the model is not a direct submodel of this coupled model, nothing will happen.
	 */
	void addSubModel(const t_modelptr& model);


	/**
	 * Adds a submodel to this coupled model
	 *
	 * @param model The submodel that is to be added
	 */
	void removeSubModel(t_modelptr& model);

	/**
	 * Resets the parent pointers of this model and all its children (depth-first)
	 */
	virtual void resetParents();

	/**
	 * Connects the given ports with eachother (with a zFunction)
	 * The zFunction will not change the message by default
	 *
	 * @param p1 pointer to output port
	 * @param p2 pointer to input port
	 * @param zFunction the zFunction that has to be executed on the message before transmitting it
	 */
	void connectPorts(const t_portptr& p1, const t_portptr& p2, t_zfunc zFunction = n_tools::createObject<ZFunc>());

	/**
	 * @brief Disconnects one connection between the two ports
	 *
	 * @param p1 pointer to output port number
	 * @param p2 pointer to input port number
	 */
	void disconnectPorts(const t_portptr& p1, const t_portptr& p2);

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
