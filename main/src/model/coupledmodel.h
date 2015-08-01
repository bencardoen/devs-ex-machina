/*
 * coupledModel.h
 *
 *  Created on: 9-mrt.-2015
 *      Author: Pieter, Tim
 */

#ifndef COUPLEDMODEL_H_
#define COUPLEDMODEL_H_

#include "model/model.h"
#include "tools/objectfactory.h"

class TestCereal;

namespace n_model {
class CoupledModel: public Model
{
	friend class ::TestCereal;
private:
	/**
	 * Removes all connections between two models
	 */
	void removeConnectionsBetween(t_modelptr& mod1, t_modelptr& mod2);

	/**
	 * Tests whether a connection from one port to the other is legal.
	 * @param p1 The port from which output is sent
	 * @param p2 The port to which the output is sent
	 * A connection p1 -> p2 is valid if:
	 * 	both p1 and p2 are either a port of a direct submodel or this model
	 * 	if the ports are both of this model or both of a submodel
	 * 		and the link is from an output port to an input port
	 * 	if the port that is not of this model has the correct direction
	 */
	bool isLegalConnection(const t_portptr& p1, const t_portptr& p2) const;

	/**
	 * Unschedule all the children
	 * @precondition: The simulation is in Dynamic Structured DEVS phase
	 */
	void unscheduleChildren();
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
	 * Removes a submodel from this coupled model
	 *
	 * @param model The submodel that is to be removed
	 * @note If the model is not a direct submodel of this coupled model, nothing will happen.
	 */
	void addSubModel(const t_modelptr& model);


	/**
	 * Adds a submodel to this coupled model
	 *
	 * @param model The submodel that is to be removed
	 * @precondition The model is owned by this Coupled Model
	 * @precondition The simulation is not running or this is in Dynamic Structured DEVS
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


	void setController(n_control::Controller* newControl) override;

	/**
	 * Serialize this object to the given archive
	 *
	 * @param archive A container for the desired output stream
	 */
	void serialize(n_serialization::t_oarchive& archive);

	/**
	 * Unserialize this object to the given archive
	 *
	 * @param archive A container for the desired input stream
	 */
	void serialize(n_serialization::t_iarchive& archive);

	/**
	 * Helper function for unserializing smart pointers to an object of this class.
	 *
	 * @param archive A container for the desired input stream
	 * @param construct A helper struct for constructing the original object
	 */
	static void load_and_construct(n_serialization::t_iarchive& archive, cereal::construct<CoupledModel>& construct);
};

typedef std::shared_ptr<CoupledModel> t_coupledmodelptr;
}

CEREAL_REGISTER_TYPE(n_model::CoupledModel)

#endif /* COUPLEDMODEL_H_ */
