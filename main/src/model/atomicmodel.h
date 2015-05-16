/*
 * atomicModel.h
 *
 *  Created on: 9-mrt.-2015
 *      Author: Pieter, Tim
 */

#ifndef ATOMICMODEL_H_
#define ATOMICMODEL_H_

#include "model.h"
#include "message.h"	// include globallog
#include <assert.h>
#include <map>
#include "globallog.h"

namespace n_model {
class AtomicModel: public Model
{
private:
	static size_t nextPriority()
	{
		static size_t initprior = 0;
		return ++initprior;
	}
	using Model::m_control;	//change access to private

	/**
	 * Core number the model wants to be on when using parallel simulation.
	 * Often used when using parallel simulation
	 * Is particularly useful when defining your models for conservative parallel optimizations
	 * Default value is -1. This value indicates no core is preferred
	 */
	int m_corenumber;

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
	 * Constructor for AtomicModel
	 *
	 * Note that 0 is the highest priority. The higher the number,
	 * the lower the priority.
	 *
	 * @param name The name of the model
	 * @param corenumber The core number that the model wants to be on
	 * @param priority The priority of the model
	 */
	AtomicModel(std::string name, int corenumber, std::size_t priority = 0);

	/**
	 * Perform an external transition, one of the functions the user has to implement
	 *
	 * @param message A vector of messagepointers that represent events
	 */
	virtual void extTransition(const std::vector<n_network::t_msgptr> & message)
	{
		LOG_ERROR(
		        "ATOMICMODEL: Not implemented: 'void n_model::AtomicModel::extTransition(const std::vector<n_network::t_msgptr> & message)'");
		assert(false);
		message.capacity();
	}
	;

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
	virtual void intTransition()
	{
		LOG_ERROR("ATOMICMODEL: Not implemented: 'void n_model::AtomicModel::intTransition()'");
		assert(false);
	}
	;

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
	virtual t_timestamp timeAdvance() const
	{
		LOG_ERROR("ATOMICMODEL: Not implemented: 't_timestamp n_model::AtomicModel::timeAdvance()'");
		assert(false);
		return t_timestamp();
	}

	/**
	 * Get the current lookahead value, one of the functions the user has to implement
	 * if he wants to make use of Parallel DEVS with conservative time synchronization
	 *
	 * @return Current lookahead value
	 */
	virtual t_timestamp lookAhead() const
	{
		LOG_WARNING(
		        "ATOMICMODEL: Lookahead: assuming 0: Not implemented: 't_timestamp n_model::AtomicModel::lookahead()'");
		return t_timestamp(0);
	}

	/**
	 * Get the current output, one of the functions the user has to implement
	 *
	 * @return vector with pointers to all output messages in it
	 */
	virtual std::vector<n_network::t_msgptr> output() const
	{
		LOG_ERROR(
		        "ATOMICMODEL: Not implemented: 'std::vector<n_network::t_msgptr> n_model::AtomicModel::output()'");
		assert(false);
		return std::vector<n_network::t_msgptr>();
	}

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
	 * @return timeNext of the model after the revert has happened
	 */
	t_timestamp revert(t_timestamp time);

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


	/**
	 * Gets the corenumber this model wants to be on
	 *
	 * @return Corenumber
	 */
	int getCorenumber() const;

	/**
	 * Sets the corenumber this model wants to be on
	 * This is a guideline, when choosing an illegal core, a different one will be appointed to this model
	 *
	 * @param corenumber The core number this model wants to be on
	 */
	void setCorenumber(int corenumber);

	virtual ~AtomicModel()
	{
	}

	/**
	 * @brief Returns the elapsed time set by the model on initialization.
	 * Elapsed time allows to shift the transitions of a model in time without having to change the TimeAdvance function.
	 */
	t_timestamp getTimeElapsed() const;

	// TODO remove stub
	void addInfluencees(std::set<std::string>& /*influences*/){
		;
	}

	/**
	 * Adds all this model's influences to the given set.
	 * This function is used in conservative parallel simulation.
	 *
	 * @param influences A set of all current influences (strings: host names) that will be completed
	 */
	void addInfluencees(std::set<std::string>& influences) const;

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
	static void load_and_construct(n_serialization::t_iarchive& archive, cereal::construct<AtomicModel>& construct);
};

typedef std::shared_ptr<AtomicModel> t_atomicmodelptr;
}	// end namespace

#endif /* ATOMICMODEL_H_ */
