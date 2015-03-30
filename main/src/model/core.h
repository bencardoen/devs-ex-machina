/*
 * core.h
 *      Author: Ben Cardoen
 */
#include "timestamp.h"
#include "network.h"
#include "atomicmodel.h"
#include "scheduler.h"
#include "terminationfunction.h"
#include "schedulerfactory.h"
#include "modelentry.h"
#include "tracers.h"

#ifndef SRC_MODEL_CORE_H_
#define SRC_MODEL_CORE_H_

namespace n_model {

using n_network::t_networkptr;
using n_network::t_msgptr;
using n_network::t_timestamp;


/**
 * Typedef used by core.
 */
typedef std::shared_ptr<n_tools::Scheduler<ModelEntry>> t_scheduler;

/**
 * A Core is a node in a parallel devs simulator. It manages (multiple) atomic models and drives their transitions.
 */
class Core
{
private:
	/**
	 * Current simulation time
	 * Loosely corresponds with Yentl's 'clock'
	 */
	t_timestamp m_time;

	/**
	 * GVT.
	 */
	t_timestamp m_gvt;

	/**
	 * Coreid, set at construction. Used by Controller/LocTable
	 */
	std::size_t m_coreid;

	/**
	 * Model storage.
	 * @attention Models are never scheduled, entries (name+time) are (as with Yentl).
	 */
	std::unordered_map<std::string, t_atomicmodelptr> m_models;

	/**
	 * Indicate if this core can/should run.
	 * @synchronized
	 */
	std::atomic<bool> m_live;

	/**
	 * Stores the model(entries) in ascending (urgent first) scheduled time.
	 */
	t_scheduler m_scheduler;

	/**
	 * Termination time, if set.
	 * @attention : if not set, infinity().
	 */
	t_timestamp m_termtime;

	/**
	 * Indicates if this core has triggered either termination condition.
	 */
	std::atomic<bool> m_terminated;

	/**
	 * Termination function.
	 * The constructor initializes this to a default functor returning false for each model (simulating forever)
	 */
	t_terminationfunctor m_termination_function;

	/**
	 * Tracers.
	 */
	n_tracers::t_tracersetptr m_tracers;
	/**
	 * Check if dest model is local, if not:
	 * Looks up message in lookuptable, set coreid.
	 * @post msg has correct destination id field set for network.
	 * @attention For single core, checks if no message has destination to model not in core (assert).
	 */
	bool
	virtual
	isMessageLocal(const t_msgptr&)const;

public:
	/**
	 * Default single core implementation.
	 * @post : coreid==0, network,loctable == nullptr., termination time=inf, termination function = null
	 */
	Core();

	Core(const Core&) = delete;

	Core& operator=(const Core&) = delete;

protected:
	Core(std::size_t id);

	/**
	 * Allow multicore implementation to directly modify time.
	 */
	void
	setTime(const t_timestamp&);

public:
	virtual ~Core() = default;

	/**
	 * Serialize this core to file fname.
	 */
	void save(const std::string& fname);

	/**
	 * Load this core from file fname;
	 */
	void load(const std::string& fname);

	/**
	 * In optimistic simulation, revert models to earlier stage defined by totime.
	 */
	virtual
	void revert(t_timestamp totime);

	/**
	 * Add model to this core.
	 */
	void addModel(t_atomicmodelptr model);

	/**
	 * Retrieve model with name from core
	 * @pre model is present in this core.
	 * @attention does not change anything in scheduled order.
	 */
	t_atomicmodelptr
	getModel(const std::string& name);

	/**
	 * Check if model is present in core.
	 */
	bool
	containsModel(const std::string& name)const;

	/**
	 * Indicates if Core is running, or halted.
	 * @synchronized
	 */
	bool isLive() const;

	/**
	 * Start/Stop core.
	 * @synchronized
	 */
	void setLive(bool live);

	/**
	 * Retrieve this core's id field.
	 */
	std::size_t getCoreID() const;

	/**
	 * Run at startup, populate the scheduler with the model's advance() results.
	 * @attention : run this once and once only.
	 */
	void init();

	/**
	 * Ask the scheduler for any model with scheduled time <= (current core time, causal::max)
	 * @attention : pops all imminent models, they need to be rescheduled (or will be lost forever).
	 */
	std::set<std::string>
	getImminent();

	/**
	 * Asks for each unscheduled model a new firing time and places items on the scheduler.
	 */
	void
	rescheduleImminent(const std::set<std::string>&);

	/**
	 * Updates local time from first entry in scheduler.
	 * @attention : if scheduler is empty this will crash hard.
	 * @post m_time >= m_time
	 */
	void
	syncTime();

	/**
	 * Run a single DEVS simulation step:
	 * 	- collect output
	 * 	- route messages (networked or not)
	 * 	- transition
	 * 	- trace
	 * 	- sync & reschedule
	 * @pre init() has run once, there exists at least 1 model that is scheduled.
	 */
	virtual
	void runSmallStep();

	/**
	 * For all models : get all messages.
	 */
	virtual void
	collectOutput(std::unordered_map<std::string, std::vector<t_msgptr>>& mailbag);

	virtual void sendMessage(const t_msgptr&)
	{
		;
	}

	/**
	 * Pull messages from network, and sort them into parameter by destination name.
	 * Base class = noop.
	 */
	virtual void getMessages(std::unordered_map<std::string, std::vector<t_msgptr>>&)
	{
		;
	}

	/**
	 * Subclass hook.
	 * If the current scheduler top time < than an event, execute any subclass logic to correct it.
	 */
	virtual void adjustTime(){;}

	/**
	 * Get Current simulation time.
	 * This is a timestamp equivalent to the first model scheduled to transition at the end of a simulation phase (step).
	 * @note The causal field is to be disregarded, it is not relevant here.
	 */
	t_timestamp getTime() const;

	/**
	 * Retrieve GVT. Only makes sense for a multi core.
	 */
	t_timestamp getGVT() const;

	/**
	 * Depending on whether a model may transition (imminent), and/or has received messages, transition.
	 * @return all transitioned models.
	 */
	virtual
	void
	transition(std::set<std::string>& imminents, std::unordered_map<std::string, std::vector<t_msgptr>>& mail);

	/**
	 * Schedule model.name @ time t.
	 * @pre Cannot be called without removing a previous scheduled entry.
	 * @TODO make private
	 */
	void
	scheduleModel(std::string name, t_timestamp t);

	/**
	 * Debug function : print out the currently scheduled models.
	 */
	void
	printSchedulerState();

	/**
	 * Given a set of messages, sort them by model destination.
	 * @attention : for single core no more than a simple sort, for multicore accesses network to push messages not local.
	 */
	virtual
	void
	sortMail(std::unordered_map<std::string, std::vector<t_msgptr>>& mailbag,
	        const std::vector<t_msgptr>& messages);

	/**
	 * Helper function, forward model to tracer.
	 */
	void
	traceInt(const t_atomicmodelptr&);

	void
	traceExt(const t_atomicmodelptr&);

	void
	traceConf(const t_atomicmodelptr&);

	/**
	 * If the current simulation time >= endtime, halt.
	 * This is checked after all transitions have happened.
	 */
	void
	setTerminationTime(t_timestamp endtime);

	t_timestamp
	getTerminationTime() const;

	/**
	 * @returns true when either termination condition is met.
	 * @attention : implies isLive == false.
	 */
	bool
	terminated() const;

	/**
	 * Set the the termination function.
	 */
	void
	setTerminationFunction(const t_terminationfunctor&);

	/**
	 * After a simulation step, verify that we need to continue.
	 */
	void
	checkTerminationFunction();

	/**
	 * Remove model from this core.
	 * @pre isLive()==false
	 * @post name is no longer scheduled/present.
	 */
	void
	removeModel(std::string name);

	/**
	 * @brief Sets the tracers that will be used from now on
	 * @precondition isLive()==false
	 */
	void
	setTracers(n_tracers::t_tracersetptr ptr);
};

typedef std::shared_ptr<Core> t_coreptr;

}

#endif /* SRC_MODEL_CORE_H_ */
