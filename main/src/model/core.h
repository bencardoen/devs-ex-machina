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
#include "messageentry.h"
#include "controlmessage.h"
#include "tracers.h"

#ifndef SRC_MODEL_CORE_H_
#define SRC_MODEL_CORE_H_

namespace n_model {

using n_network::t_networkptr;
using n_network::t_msgptr;
using n_network::t_timestamp;


//typedef std::priority_queue<t_msgptr, std::deque<t_msgptr>, compare_msgptr> t_msgqueue;

/**
 * Typedefs used by core.
 */
typedef std::shared_ptr<n_tools::Scheduler<ModelEntry>> t_scheduler;
typedef std::shared_ptr<n_tools::Scheduler<MessageEntry>> t_msgscheduler;

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
	 * Indicate if this core is beyond termination, but waiting on others.
	 */
	std::atomic<bool> m_idle;

	/**
	 * Check if dest model is local, if not:
	 * Looks up message in lookuptable, set coreid.
	 * @post msg has correct destination id field set for network.
	 * @attention For single core, checks if no message has destination to model not in core (assert).
	 */
	bool
	virtual
	isMessageLocal(const t_msgptr&)const;

	/**
	 * After a simulation step, verify that we need to continue.
	 */
	void
	checkTerminationFunction();


	virtual
	void
	lockSimulatorStep(){
		;
	}

	virtual
	void
	unlockSimulatorStep(){
		;
	}

	virtual
	void
	lockMessages(){;}

	virtual
	void
	unlockMessages(){;}

	/**
	 * Schedule model.name @ time t.
	 * @pre Cannot be called without removing a previous scheduled entry.
	 */
	void
	scheduleModel(std::string name, t_timestamp t);

protected:
	/**
	 * Model storage.
	 * @attention Models are never scheduled, entries (name+time) are (as with Yentl).
	 */
	std::unordered_map<std::string, t_atomicmodelptr> m_models;

	/**
	* Store received messages (local and networked)
	*/
	t_msgscheduler	m_received_messages;

	/**
	 * Push msg onto pending stack of msgs. Called by revert, receive.
	 * @lock Unlocked (ie locked by caller)
	 */
	void queuePendingMessage(const t_msgptr& msg);

	/**
	 * Constructor intended for subclass usage only. Same initialization semantics as default constructor.
	 */
	Core(std::size_t id);

	/**
	 * Subclass hook. Is called after imminents are collected.
	 * Superclass does nothing.
	 */
	virtual
	void
	signalImminent(const std::set<std::string>& ){;}

	/**
	 * In case of a revert, wipe the scheduler clean, inform all models of the changed time and reload the scheduler
	 * with fresh entries.
	 */
	void
	rescheduleAll(const t_timestamp& totime);

	/**
	 * Called by subclasses, undo tracing up to a time < totime, with totime >= gvt.
	 */
	void
	revertTracerUntil(const t_timestamp& totime);

public:
	/**
	 * Default single core implementation.
	 * @post : coreid==0, network,loctable == nullptr., termination time=inf, termination function = null
	 */
	Core();

	Core(const Core&) = delete;

	Core& operator=(const Core&) = delete;


	/**
	 * The destructor explicitly resets all shared_ptrs kept in this core (to models, msgs)
	 */
	virtual ~Core();

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
	 * @pre totime >= this->getGVT() && totime < this->getTime()
	 */
	virtual
	void revert(const t_timestamp& /*totime*/){;}

	/**
	 * Add model to this core.
	 * @pre !containsModel(model->getName());
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
	 * @return true if a Core has reached a termination condition, and is potentially waiting for
	 * other cores to finish. != isLive().
	 */
	virtual
	bool isIdle() const;

	/**
	 * Mark this core as having reached termination condition, but keep it alive (waiting for
	 * other cores).
	 */
	virtual
	void
	setIdle(bool idlestate);

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
	 * Sets earliests possible time for all models.
	 * @attention : run this once and once only. Multiple runs can trigger asserts, which will hang the
	 * process in multithreaded setting.
	 */
	void init();

	/**
	 * Ask the scheduler for any model with scheduled time <= (current core time, causal::max)
	 * @attention : pops all imminent models, they need to be rescheduled (or will be lost forever).
	 */
	std::set<std::string>
	getImminent();

	/**
	 * Called in case of Dynamic structured Devs.
	 * Stores imminent models into parameter (which is cleared first)
	 * @attention : noop in superclass
	 */
	virtual
	void
	getLastImminents(std::vector<t_atomicmodelptr>&){
		assert(false && "Not supported in non dynamic structured devs");
	}

	/**
	 * Asks for each unscheduled model a new firing time and places items on the scheduler.
	 */
	void
	rescheduleImminent(const std::set<std::string>&);

	/**
	 * Updates local time. The core time will advance to min(first transition, earliest received message).
	 */
	void
	syncTime();

	/**
	 * Allow multicore implementation to directly modify time. (GVT etc)
	 */
	void
	setTime(const t_timestamp&);

	/**
	 * Run a single DEVS simulation step:
	 * 	- collect output
	 * 	- route messages (networked or not)
	 * 	- transition
	 * 	- trace
	 * 	- reschedule fired models
	 * 	- update core time to furthest point possible
	 * @pre init() has run once, there exists at least 1 model that is scheduled.
	 * @return Models who have transitioned (internal or confluent)
	 * @attention null return value for superclass.
	 */
	virtual
	void
	runSmallStep();

	/**
	 * Collect output from all models, sort them in the mailbag by destination name.
	 * @attention : generated messages (events) are timestamped by the current core time.
	 */
	virtual void
	collectOutput(std::set<std::string>& imminents);

	/**
	 * Hook for subclasses to override. Called whenever a message for the net is found.
	 */
	virtual void sendMessage(const t_msgptr&)
	{
		assert(false && "A message for a remote core in a single core implemenation.");
	}

	/**
	 * Pull messages from network, and sort them into parameter by destination name.
	 * Base class = noop.
	 */
	virtual void getMessages()
	{
		;
	}

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
	 * Set current GVT
	 * @lock simulator, messages in multicore.
	 */
	virtual void
	setGVT(const t_timestamp& newgvt);

	/**
	 * Depending on whether a model may transition (imminent), and/or has received messages, transition.
	 * @param imminent modelnames with firing time == to current time
	 * @param mail collected from local/network by collectOutput/getMessages
	 */
	virtual
	void
	transition(std::set<std::string>& imminents, std::unordered_map<std::string, std::vector<t_msgptr>>& mail);

	/**
	 * Debug function : print out the currently scheduled models.
	 */
	void
	printSchedulerState();

	/**
	 * Print all queued messages.
	 * @attention : invokes a full copy of all stored msg ptrs, only for debugging!
	 * @lock : locks on messages
	 */
	void
	printPendingMessages();

	/**
	 * Given a set of messages, sort them by model destination.
	 * @attention : for single core no more than a simple sort, for multicore accesses network to push messages not local.
	 * @lock: locks on messagelock.
	 */
	virtual
	void
	sortMail(const std::vector<t_msgptr>& messages);

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

	void
	setTerminated(bool b);

	/**
	 * Set the the termination function.
	 */
	void
	setTerminationFunction(const t_terminationfunctor&);

	/**
	 * Remove model with specified name from this core (if present).
	 * This removes the model, unschedules it (if it is scheduled). It does
	 * not remove queued messages for this model, but the core takes this into
	 * account.
	 * @attention : call only in single core or if core is not live.
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

	/**
	 * Signal tracers to flush output up to a given time.
	 * For the single core implementation this is the local time.
	 * @attention : this can only be run IF all cores are stopped. !!!!!
	 */
	virtual
	void
	signalTracersFlush()const;

	/**
	 * Remove all models from this core.
	 * @pre (not this->isLive())
	 * @post this->getTime()==t_timestamp(0,0) (same for gvt)
	 * @attention : DO NOT invoke this once simulation has started (timewarp!!)
	 */
	void
	clearModels();

	/**
	 * Sort message in individual receiving queue.
	 */
	virtual
	void receiveMessage(const t_msgptr&);

	/**
	 * Get the mail with timestamp < nowtime sorted by destination.
	 * @locks on messagelock
	 */
	virtual
	void getPendingMail(std::unordered_map<std::string, std::vector<t_msgptr>>&);

	/**
	 * After a model received a set of messages, store these for later use.
	 * @attention noop in single core, only relevant in multicore (revert)
	 */
	virtual
	void markProcessed(const std::vector<t_msgptr>&) {;}

	// TODO make private
	virtual
	void markMessageStored(const t_msgptr&){;}

	/**
	 * For all pending messages, retrieve the smallest (earliest) timestamp.
	 * @return earliest timestamp of pending messages, or infinity() if no usch time is found.
	 * @locks on messagelock
	 */
	t_timestamp
	getFirstMessageTime();


	/**
	 * Mattern's algorithm nrs 1.6/1.7
	 * @attention : only sensible in multicore setting, single core will assert fail.
	 */
	virtual
	void
	receiveControl(const t_controlmsg& /*controlmessage*/, bool /*first*/){
		assert(false);
	}

	/**
	 * Noop in single core (and is never called), but it is called as base trigger by receiveMessage()
	 */
	virtual
	void
	handleAntiMessage(const t_msgptr&){;}

	/**
	 * Noop in single core. Paints message according to current color in core. (multicore).
	 */
	virtual
	void
	paintMessage(const t_msgptr&){;}

	void
	logCoreState();
};

typedef std::shared_ptr<Core> t_coreptr;

}

#endif /* SRC_MODEL_CORE_H_ */
