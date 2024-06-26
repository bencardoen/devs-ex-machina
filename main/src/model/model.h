/*
 * This file is part of the DEVS Ex Machina project.
 * Copyright 2014 - 2015 University of Antwerp 
 * https://www.uantwerpen.be/en/
 * Licensed under the EUPL V.1.1
 * A full copy of the license is in COPYING.txt, or can be found at 
 * https://joinup.ec.europa.eu/community/eupl/og_page/eupl 
 *      Author: Stijn Manhaeve, Ben Cardoen, Tim Tuijn
 */

#ifndef MODEL_H_
#define MODEL_H_

#include <map>
#include <vector>
#include <deque>
#include <memory>
#include <sstream>
#include "model/port.h"		//include glog, ofactory
#include "model/state.h"
#include "model/dssharedstate.h"	//include string

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
	bool removedInPort;
	bool removedOutPort;

	/**
	 * Utility function to create a new port and add it
	 *
	 * @param name The name of the port
	 * @param isIn Whether or not this port is an input port
	 */
	t_portptr addPort(std::string name, bool isIn);

protected:

	std::vector<t_portptr> m_iPorts;
	std::vector<t_portptr> m_oPorts;

	n_control::Controller* m_control;

	/**
	 * Parent node of this model, mainly used for direct connect and DS
	 * Weak pointer is used for easier destruction
	 */
	Model* m_parent;

	/**
	 * Add an input port to the model
	 *
	 * @param name The name of the port
	 */
        t_portptr addInPort(const std::string& name);

	/**
	 * Add an output port to the model
	 *
	 * @param name The name of the port
	 */
        t_portptr addOutPort(const std::string& name);

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
	const t_portptr& getPort(std::string name) const;

	/**
	 * @brief Removes a port from this model.
	 */
        void removePort(t_portptr& port);

	/**
	 * @brief Removes all incoming and outgoing connections on this model.
	 */
	void clearConnections();

	/**
	 * Sets the current parent pointer to parent
	 *
	 * @param parent new parent pointer
	 */
	void setParent(Model* parent);

	/**
	 * Get the current parent pointer
	 *
	 * @return current parent pointer or nullptr if parent is not set
	 */
	const Model* getParent() const;
	Model* getParent();

	/**
	 * Resets the parent pointers of this model
	 */
	void resetParent();

	/**
	 * Return all current input ports
	 *
	 * @return current input ports
	 */
	const std::vector<t_portptr>& getIPorts() const;

	/**
	 * Return all current output ports
	 *
	 * @return current output ports
	 */
	const std::vector<t_portptr>& getOPorts() const;

	/**
	 * Return all current input ports
	 *
	 * @return current input ports
	 */
	std::vector<t_portptr>& getIPorts();

	/**
	 * Return all current output ports
	 *
	 * @return current output ports
	 */
	std::vector<t_portptr>& getOPorts();

	/**
	 * @brief Transition function for dynamic structured DEVS.
	 * This function will be called during the simulation for changing the structure of the model.
	 * @return If true, propagate this call upwards in the model tree.
	 * @note Only this function is allowed to change the structure during the simulation.
	 * @note This function will automatically call the user defined function modelTransition.
	 */
	bool doModelTransition(DSSharedState* shared);

	/**
	 * @brief Transition function for dynamic structured DEVS.
	 * This function will be called during the simulation for changing the structure of the model.
	 * @return If true, propagate this call upwards in the model tree.
	 * @note Only this function is allowed to change the structure during the simulation.
	 */
	virtual bool modelTransition(DSSharedState* shared);

	/**
	 * @brief Sets the Controller of this model.
	 * @note The user doesn't have to worry about this one.
	 */
	virtual void setController(n_control::Controller* newControl);
	

//-------------statistics gathering--------------
public:
	/**
	 * @brief Prints some basic stats.
	 * @param out The output will be printed to this stream.
	 */
	virtual void printStats(std::ostream& out = std::cout) const
	{
#ifdef USE_STAT
		for(const auto& i: m_oPorts)
			i->printStats(out);
#endif
	}
};

typedef std::shared_ptr<Model> t_modelptr;
}

#endif /* MODEL_H_ */
