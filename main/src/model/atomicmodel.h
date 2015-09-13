/*
 * atomicModel.h
 *
 *  Created on: 9-mrt.-2015
 *      Author: Pieter, Tim
 */

#ifndef ATOMICMODEL_H_
#define ATOMICMODEL_H_

#include "model/model.h"
#include "model/uuid.h"
#include "network/message.h"	// include globallog
#include <assert.h>
#include <map>
#include "tools/globallog.h"
#include "serialization/archive.h"
#include "cereal/types/polymorphic.hpp"
#include "cereal/access.hpp"
#include <set>

class TestCereal;

namespace n_model {


class AtomicModel: public Model
{
	friend class ::TestCereal;
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
        
        /**
         * Stores this model unique identifier.
         */
        uuid    m_uuid;

protected:
	// lower number -> higher priority
	std::size_t m_priority;
	t_timestamp m_elapsed;
	t_timestamp m_lastRead;
        
        /**
	 * Add an input port to the model
	 * Link the port to this model
	 * @param name The name of the port
	 */
        virtual
	t_portptr addInPort(std::string name)override final;

	/**
	 * Add an output port to the model
	 * Link the port to this model
	 * @param name The name of the port
	 */
        virtual
	t_portptr addOutPort(std::string name)override final;

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
	AtomicModel(std::string name, std::size_t priority = std::numeric_limits<size_t>::max());

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
	AtomicModel(std::string name, int corenumber, std::size_t priority = std::numeric_limits<size_t>::max());
        
        /**
         * @return uuid object.
         * @attention This is only initialized after a model is added to a core AND 
         * the init function is invoked on that core (ie right before simulation starts).
         */
        uuid& getUUID()
        {
                return m_uuid;
        }
        
        /**
         * Called by the core before simulation starts.
         * @param cid Core identifier
         * @param lid Local identifier
         */
        void initUUID(size_t cid, size_t lid){
                m_uuid.m_local_id=lid;
                m_uuid.m_core_id=cid;
        }
        
        size_t getLocalID() const{
                return m_uuid.m_local_id;
        }
        
        size_t getCoreID()const{
                assert(m_uuid.m_core_id != (size_t) m_corenumber && "Core id corrupt");// if cnr = -1, still fine to compare (2^64 - 1)
                return m_uuid.m_core_id;
        }

	/**
	 * Perform an external transition
	 *
	 * @param message A vector of messagepointers that represent events
	 * @warning This function MUST be implemented in the simulated model
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
	 * Perform an internal transition, this function will call the user-implemented intTransition
	 * function and will also store all messages properly for the tracer to find them
	 */
	void doIntTransition();

	/**
	 * Perform an internal transition.
	 * @warning This function MUST be implemented in the simulated model
	 */
	virtual void intTransition()
	{
		LOG_ERROR("ATOMICMODEL: Not implemented: 'void n_model::AtomicModel::intTransition()'");
		assert(false);
	}
	;

	/**
	 * Transitions the model confluently with given messages.
	 * The default implementation will first call intTransition and then extTransition.
	 * If a different implementation is needed, the user can provide it by
	 * overriding this funtion.
	 *
	 * @param message List of messages that are needed for the transition
	 */
	virtual void confTransition(const std::vector<n_network::t_msgptr> & message);

	/**
	 * Transitions the model confluently with given messages, this function will call the user-implemented confTransition
	 * function and will also store all messages properly for the tracer to find them
	 *
	 * @param message List of messages that are needed for the transition
	 */
	void doConfTransition(const std::vector<n_network::t_msgptr> & message);

	/**
	 * Get the current time advance
	 *
	 * @return Current time advance
	 * @warning This function MUST be implemented in the simulated model
	 * @postcondition The time advance may not be 0 or negative.
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
	 * Get the current output
	 *
	 * @return vector with pointers to all output messages in it
	 * @warning This function MUST be implemented in the simulated model
	 * @note Keep in mind that this function is called before
	 * 	any of the transition functions are called.
	 */
	virtual void output(std::vector<n_network::t_msgptr>&) const
	{
		LOG_ERROR(
		        "ATOMICMODEL: Not implemented: 'std::vector<n_network::t_msgptr> n_model::AtomicModel::output()'");
		assert(false);
//		return std::vector<n_network::t_msgptr>();
	}

	/**
	 * Get the current output, this function will call the user-implemented output function
	 * and will also store all messages properly for the tracer to find them.
	 *
	 * @return vector with pointers to all output messages in it
	 */
	void doOutput(std::vector<n_network::t_msgptr>& msgs);

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
	
	/**
	 * Adds all this model's influences to the given set.
	 * This function is used in conservative parallel simulation.
	 *
	 * @param influences A set of all current influences (strings: host names) that will be completed
	 */
	void addInfluencees(std::set<std::string>& influences) const;

	/**
	 * @brief Sets the elapsed time since the previous internal transition.
	 */
	void setTimeElapsed(t_timestamp elapsed);
        
        /**
	 * @brief Removes a port from this model.
	 */
        virtual
	void removePort(t_portptr& port)override final;

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

CEREAL_REGISTER_TYPE(n_model::AtomicModel)

#endif /* ATOMICMODEL_H_ */
