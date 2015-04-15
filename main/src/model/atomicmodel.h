/*
 * atomicModel.h
 *
 *  Created on: 9-mrt.-2015
 *      Author: Pieter, Tim
 */

#ifndef ATOMICMODEL_H_
#define ATOMICMODEL_H_

#include "model.h"
#include "message.h"
#include <map>
#include <iostream>

namespace n_model {
class AtomicModel: public Model
{
protected:
	// lower number -> higher priority
	std::size_t m_priority;
	t_timestamp m_elapsed;
	t_timestamp m_lastRead;

public:
	AtomicModel() = delete;

	/**
	 * Constructor for AtomicModel
	 *
	 * Note that 0 is the highest priority. The higher the number,
	 * the lower the priority.
	 *
	 * @param name The name of the model
	 * @param priority The priority of the model
	 */
	AtomicModel(std::string name, std::size_t priority = 0);

	/**
	 * Perform an external transition, one of the functions the user has to implement
	 *
	 * @param message A vector of messagepointers that represent events
	 */
	virtual void extTransition(const std::vector<n_network::t_msgptr> & message) = 0;

	/**
	 * Perform an external transition, this function will call the user-implemented extTransition
	 * function and will also store all messages properly for the tracer to find them
	 *
	 * @param message A vector of messagepointers that represent events
	 */
	void doExtTransition(const std::vector<n_network::t_msgptr>& message);

	/**
	 * Perform an internal transition, one of the functions the user has to implement
	 */
	virtual void intTransition() = 0;

	/**
	 * Transitions the model confluently with given messages
	 *
	 * @param message List of messages that are needed for the transition
	 */
	virtual void confTransition(const std::vector<n_network::t_msgptr> & message);

	/**
	 * Get the current time advance, one of the functions the user has to implement
	 *
	 * @return Current time advance
	 */
	virtual t_timestamp timeAdvance() = 0;

	/**
	 * Get the current output, one of the functions the user has to implement
	 *
	 * @return vector with pointers to all output messages in it
	 */
	virtual std::vector<n_network::t_msgptr> output() const = 0;

	/**
	 * Get the current output, this function will call the user-implemented output function
	 * and will also store all messages properly for the tracer to find them.
	 *
	 * @return vector with pointers to all output messages in it
	 */
	std::vector<n_network::t_msgptr> doOutput();

	/**
	 * Sets the correct GVT for the model and fixes all necessary internal changes (like
	 * the removal of old states that aren't necessary anymore)
	 *
	 * @param gvt The gvt that we need to reset to
	 */
	void setGVT(t_timestamp gvt);

	/**
	 * Reverts the model the given time
	 *
	 * @param time The time the model needs to be reverted to
	 */
	void revert(t_timestamp time);

	/**
	 * Returns the priority of the model
	 *
	 * @return priority of model
	 */
	std::size_t getPriority() const;

	/**
	 * Sets the correct time of the model after a transition has happened.
	 * This function has to be called immediately after a transition!
	 *
	 * @param time The current time of the simulation
	 */
	void setTime(t_timestamp time);

	virtual ~AtomicModel()
	{
	}
};

typedef std::shared_ptr<AtomicModel> t_atomicmodelptr;
}

#endif /* ATOMICMODEL_H_ */
