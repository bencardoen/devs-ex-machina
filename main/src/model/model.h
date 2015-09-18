/*
 * model.h
 *
 *  Created on: 9-mrt.-2015
 *      Author: Pieter, Tim
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

class TestCereal;

namespace n_model {

using n_network::t_timestamp;


class Model
{
	friend n_control::Controller;
	friend class ::TestCereal;
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

	std::map<std::string, t_portptr> m_iPorts;
	std::map<std::string, t_portptr> m_oPorts;

	n_control::Controller* m_control;

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
        virtual
	t_portptr addInPort(std::string name);

	/**
	 * Add an output port to the model
	 *
	 * @param name The name of the port
	 */
        virtual
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
        virtual
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
	virtual bool modelTransition(DSSharedState* shared);

	/**
	 * @brief Sets the Controller of this model.
	 * @note The user doesn't have to worry about this one.
	 */
	virtual void setController(n_control::Controller* newControl);

//-------------serialization---------------------
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
	static void load_and_construct(n_serialization::t_iarchive& archive, cereal::construct<Model>& construct);


//-------------statistics gathering--------------
//#ifdef USE_STAT
public:
	/**
	 * @brief Prints some basic stats.
	 * @param out The output will be printed to this stream.
	 */
	virtual void printStats(std::ostream& out = std::cout) const
	{
		for(const auto& i: m_oPorts)
			i.second->printStats(out);
	}
//#endif
};

typedef std::shared_ptr<Model> t_modelptr;
}

#endif /* MODEL_H_ */
