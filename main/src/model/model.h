/*
 * model.h
 *
 *  Created on: 9-mrt.-2015
 *      Author: Pieter, Tim
 */

#ifndef MODEL_H_
#define MODEL_H_

#include <string>
#include <map>
#include <vector>
#include <deque>
#include <memory>
#include <sstream>
#include "port.h"
#include "state.h"
#include "dssharedstate.h"

namespace n_control{
	class Controller;
}

namespace n_model {

using n_network::t_timestamp;

class Model
{
	friend n_control::Controller;
private:
	std::string m_name;

	/**
	 * Utility function to create a new port and add it
	 *
	 * @param name The name of the port
	 * @param isIn Whether or not this port is an input port
	 */
	t_portptr addPort(std::string name, bool isIn);

protected:
	t_timestamp m_timeLast;
	t_timestamp m_timeNext;

	t_stateptr m_state;
	std::vector<t_stateptr> m_oldStates;

	std::map<std::string, t_portptr> m_iPorts;
	std::map<std::string, t_portptr> m_oPorts;

	std::deque<n_network::t_msgptr> m_sendMessages;
	std::deque<n_network::t_msgptr> m_receivedMessages;

	n_control::Controller* m_control;	//@Pieter Deze member moet je niet serializeren.

	/**
	 * Parent node of this model, mainly used for direct connect and DS
	 * Weak pointer is used for easier destruction
	 */
	std::weak_ptr<Model> m_parent;

	/**
	 * Add an input port to the model
	 *
	 * @param name The name of the port
	 */
	t_portptr addInPort(std::string name);

	/**
	 * Add an output port to the model
	 *
	 * @param name The name of the port
	 */
	t_portptr addOutPort(std::string name);

	/**
	 * @return Whether or not to allow structural changes
	 */
	bool allowDS() const;

public:
	Model() = delete;

	/**
	 * Constructor for Model
	 *
	 * @param name of model
	 */
	Model(std::string name);

	/**
	 * Virtual destructor for Model
	 */
	virtual ~Model()
	{
		// Delete all shared pointers to ports so they too can be freed
		for (auto& p : m_iPorts) {
			p.second->clear();

		}
		for (auto& p : m_oPorts) {
			p.second->clear();
		}
	}

	/**
	 * Returns the name of the model
	 *
	 * @return name of model
	 */
	std::string getName() const;

	/**
	 * Returns the port corresponding with the given name
	 *
	 * @param name The name of the port
	 * @return a shared pointer to the port
	 */
	t_portptr getPort(std::string name) const;

	/**
	 * @brief Removes a port from this model.
	 */
	void removePort(t_portptr& port);

	/**
	 * Returns the current state of the model
	 *
	 * @return current state of model
	 */
	t_stateptr getState() const;

	/**
	 * Set the current state of the model to a new state and pushes this new state on
	 * the list of all oldStates.
	 *
	 * @param newState the new state the model should switch to (as a State object)
	 */
	void setState(const t_stateptr& newState);

	/**
	 * Sets the current parent pointer to parent
	 *
	 * @param parent new parent pointer
	 */
	void setParent(const std::shared_ptr<Model>& parent);

	/**
	 * Get the current parent pointer
	 *
	 * @return current parent pointer or nullptr if parent is not set
	 */
	const std::weak_ptr<Model>& getParent() const;

	/**
	 * Resets the parent pointers of this model
	 */
	virtual void resetParents();

	/**
	 * Return all current input ports
	 *
	 * @return current input ports
	 */
	const std::map<std::string, t_portptr>& getIPorts() const;

	/**
	 * Return all current output ports
	 *
	 * @return current output ports
	 */
	const std::map<std::string, t_portptr>& getOPorts() const;

	/**
	 * Return all current input ports
	 *
	 * @return current input ports
	 */
	std::map<std::string, t_portptr>& getIPorts();

	/**
	 * Return all current output ports
	 *
	 * @return current output ports
	 */
	std::map<std::string, t_portptr>& getOPorts();

	/**
	 * @brief Transition function for dynamic structured DEVS.
	 * This function will be called during the simulation for changing the structure of the model.
	 * @return If true, propagate this call upwards in the model tree.
	 * @note Only this function is allowed to change the structure during the simulation.
	 */
	virtual bool modelTransition(DSScharedState* shared);

	/**
	 * @brief Sets the Controller of this model.
	 * @note The user doesn't have to worry about this one.
	 */
	virtual void setController(n_control::Controller* newControl);
};

typedef std::shared_ptr<Model> t_modelptr;
}

#endif /* MODEL_H_ */
