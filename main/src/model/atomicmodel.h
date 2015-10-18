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

typedef uint8_t t_transtype;

constexpr t_transtype NONE=0;
constexpr t_transtype EXT=1;
constexpr t_transtype INT=2;
constexpr t_transtype CONF=3;   // EXT | INT

class AtomicModel_impl: public Model
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

	/**
	 * @brief Variable that determines if old states are to be kept or not
	 * It is sometimes useful to set this variable false if you only want to run your
	 * AtomicModel single-core.
	 * @note This is not to be used when simulating coupled models or models in parallel!
	 */
	bool m_keepOldStates;

	t_timestamp m_timeLast;
	t_timestamp m_timeNext;

	t_stateptr m_state;
	std::vector<t_stateptr> m_oldStates;

protected:
	// lower number -> higher priority
	std::size_t m_priority;

private:        
        /**
         * Record if this model has an external transition pending.
         */
        t_transtype m_transition_type_next;

	/**
	 * @brief Copies the state, if necessary
	 */
	void copyState();

	/**
	 * @brief delivers all the messages to the correct port
	 */
	void deliverMessages(const std::vector<n_network::t_msgptr>& message);

protected:
	
	t_timestamp m_elapsed;
	t_timestamp m_lastRead;

public:
	AtomicModel_impl() = delete;

	/**
	 * Constructor for AtomicModel_impl
	 *
	 * Note that 0 is the highest priority. The higher the number,
	 * the lower the priority.
	 *
	 * @param name The name of the model
	 * @param priority The priority of the model
	 */
	AtomicModel_impl(std::string name, std::size_t priority = std::numeric_limits<size_t>::max());

	/**
	 * Constructor for AtomicModel_impl
	 *
	 * Note that 0 is the highest priority. The higher the number,
	 * the lower the priority.
	 *
	 * @param name The name of the model
	 * @param corenumber The core number that the model wants to be on
	 * @param priority The priority of the model
	 */
	AtomicModel_impl(std::string name, int corenumber, std::size_t priority = std::numeric_limits<size_t>::max());
        
	virtual ~AtomicModel_impl() = default;

        /**
         * @return uuid object.
         * @attention This is only initialized after a model is added to a core AND 
         * the init function is invoked on that core (ie right before simulation starts).
         */
        uuid& getUUID()
        {
                return m_uuid;
        }
        
        t_transtype&
        nextType(){
        	LOG_DEBUG("model ", getName(), " getting the next type ", int(m_transition_type_next));
        	return m_transition_type_next;}
        
        const t_transtype&
        nextType()const{return m_transition_type_next;}
        
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
                assert(m_uuid.m_core_id == (size_t) m_corenumber && "Core id corrupt");// if cnr = -1, still fine to compare (2^64 - 1)
                return m_uuid.m_core_id;
        }

	/**
	 * @brief Gets the next scheduled time.
	 */
	t_timestamp getTimeNext() const;

	/**
	 * @brief Gets the last scheduled time.
	 */
	t_timestamp getTimeLast() const;

	/**
	 * @brief Sets the variable that determines if old states are to be kept or not
	 * It is sometimes useful to set this variable false if you only want to run your
	 * AtomicModel single-core.
	 * @note This is not to be used when simulating coupled models or models in parallel!
	 */
	void setKeepOldStates(bool b);

	/**
	 * @brief Gets the variable that determines if old states are to be kept or not
	 * @return True or False
	 */
	bool getKeepOldStates() const;

	/**
	 * Returns the current state of the model
	 *
	 * @return current state of model
	 */
	const t_stateptr& getState() const;
	/**
	 * Returns the current state of the model
	 *
	 * @return current state of model
	 */
	t_stateptr& getState();

	/**
	 * @brief Initializes the atomic model with this state.
	 * @param stateptr A pointer to the initial state
	 * @precondition The pointer is a valid pointer.
	 */
	void initState(const t_stateptr& stateptr);

	/**
	 * Set the current state of the model to a new state and pushes this new state on
	 * the list of all oldStates.
	 *
	 * @param newState the new state the model should switch to (as a State object)
	 */
//	void setState(const t_stateptr& newState);

	/**
	 * Perform an external transition
	 *
	 * @param message A vector of message pointers that represent events
	 * @warning This function MUST be implemented in the simulated model
	 */
	virtual void extTransition(const std::vector<n_network::t_msgptr> & message);

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
	virtual void intTransition();

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
		LOG_ERROR("ATOMICMODEL: Not implemented: 't_timestamp n_model::AtomicModel_impl::timeAdvance()'");
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
		        "ATOMICMODEL: Lookahead: assuming 0: Not implemented: 't_timestamp n_model::AtomicModel_impl::lookahead()'");
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
		        "ATOMICMODEL: Not implemented: 'std::vector<n_network::t_msgptr> n_model::AtomicModel_impl::output()'");
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
	void addInfluencees(std::vector<std::size_t>& influences) const;

	/**
	 * @brief Sets the elapsed time since the previous internal transition.
	 */
	void setTimeElapsed(t_timestamp elapsed);

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
	static void load_and_construct(n_serialization::t_iarchive& archive, cereal::construct<AtomicModel_impl>& construct);
};

typedef std::shared_ptr<AtomicModel_impl> t_atomicmodelptr;


template<typename T>
class AtomicModel: public AtomicModel_impl
{
public:
	typedef T t_type;
private:
public:
	AtomicModel(std::string name, const T& value, std::size_t priority = 0):
		AtomicModel_impl(name, priority)
	{
		initState(n_tools::createObject<State__impl<t_type>>(value));
	}
	AtomicModel(std::string name, const T& value, int coreNum, std::size_t priority = 0):
		AtomicModel_impl(name, coreNum, priority)
	{
		initState(n_tools::createObject<State__impl<t_type>>(value));
	}
	AtomicModel(std::string name, std::size_t priority = 0):
		AtomicModel_impl(name, priority)
	{
		initState(n_tools::createObject<State__impl<t_type>>());
	}
	AtomicModel(std::string name, int coreNum, std::size_t priority = 0):
		AtomicModel_impl(name, coreNum, priority)
	{
		initState(n_tools::createObject<State__impl<t_type>>());
	}

	/**
	 * @brief Returns a reference to the current state.
	 */
	constexpr const t_type& state() const
	{
		return n_tools::staticCast<const State__impl<t_type>>(getState())->m_value;
	}

	/**
	 * @brief Returns a reference to the current state.
	 */
	t_type& state()
	{
		return n_tools::staticCast<State__impl<t_type>>(getState())->m_value;
	}

};

/**
 * @brief Specialization for when the model has no internal state.
 */
template<>
class AtomicModel<void>: public AtomicModel_impl
{
public:
	AtomicModel(std::string name, std::size_t priority = 0):
		AtomicModel_impl(name, priority)
	{
		initState(n_tools::createObject<State__impl<void>>());
	}
	AtomicModel(std::string name, int coreNum, std::size_t priority = 0):
		AtomicModel_impl(name, coreNum, priority)
	{
		initState(n_tools::createObject<State__impl<void>>());
	}
};
}	// end namespace

CEREAL_REGISTER_TYPE(n_model::AtomicModel_impl)

#endif /* ATOMICMODEL_H_ */
