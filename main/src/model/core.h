/*
 * This file is part of the DEVS Ex Machina project.
 * Copyright 2014 - 2015 University of Antwerp 
 * https://www.uantwerpen.be/en/
 * Licensed under the EUPL V.1.1
 * A full copy of the license is in COPYING.txt, or can be found at 
 * https://joinup.ec.europa.eu/community/eupl/og_page/eupl 
 *      Author: Stijn Manhaeve, Ben Cardoen, Tim Tuijn
 */
#include "model/modelentry.h"
#include "model/terminationfunction.h"		// include atomicmodel
#include "network/controlmessage.h"
#include "network/messageentry.h"
#include "network/network.h"
#include "scheduler/modelscheduler.h"
#include "scheduler/schedulerfactory.h"
#include "tools/gviz.h"
#include "tools/statistic.h"
#include "tracers/tracers.h"
#include <set>
#include <condition_variable>

#ifndef SRC_MODEL_CORE_H_
#define SRC_MODEL_CORE_H_

namespace n_model {


using n_network::t_networkptr;
using n_network::t_msgptr;
using n_network::t_timestamp;


enum STAT_TYPE{MSGSENT,MSGRCVD,AMSGSENT,AMSGRCVD,TURNS,REVERTS,STALLEDROUNDS, DELMSG};

/**
 * Typedefs used by core.
 */
typedef n_scheduler::t_defaultModelScheduler t_scheduler;
typedef std::shared_ptr<n_scheduler::Scheduler<MessageEntry>> t_msgscheduler;

struct statistics_collector{
        n_tools::t_uintstat     m_amsg_sent;
        n_tools::t_uintstat     m_amsg_rcvd;
        n_tools::t_uintstat     m_turns;
        n_tools::t_uintstat     m_turns_stalled;
        n_tools::t_uintstat     m_reverts;
        n_tools::t_uintstat     m_msgs_sent;
        n_tools::t_uintstat     m_msgs_rcvd;
        n_tools::t_uintstat     m_deleted_msgs;
        static std::string getName(std::size_t id, std::string name){
        	return std::string("_core") + n_tools::toString(id) + "/" + name;
        }
        statistics_collector(std::size_t id):
        	m_amsg_sent(getName(id, "anti_send"), "messages"),
        	m_amsg_rcvd(getName(id, "anti_received"), "messages"),
        	m_turns(getName(id, "turns"), ""),
                m_turns_stalled(getName(id, "stalled"), "turns"),
        	m_reverts(getName(id, "reverts"), ""),
        	m_msgs_sent(getName(id, "send"), "messages"),
        	m_msgs_rcvd(getName(id, "received"), "messages"),
                m_deleted_msgs(getName(id, "deleted"),"messages")
        {;}
        void printStats(std::ostream& out = std::cout) const noexcept
        {
                try{
			out << m_amsg_sent
				<< m_amsg_rcvd
				<< m_turns
				<< m_turns_stalled
				<< m_msgs_sent
				<< m_msgs_rcvd
				<< m_reverts
                                << m_deleted_msgs;
                }catch(...){
                        LOG_ERROR("Exception caught in printStats()");
                }
        }
        void logStat(enum STAT_TYPE st){
#ifdef USE_STAT
                switch(st){
                case MSGRCVD:{
                        ++m_msgs_rcvd;
                        break;
                }
                case MSGSENT:{
                        ++m_msgs_sent;
                        break;
                }
                case AMSGRCVD:{
                        ++m_amsg_rcvd;
                        break;
                }
                case AMSGSENT:{
                        ++m_amsg_sent;
                        break;
                }
                case TURNS:{
                        ++m_turns;
                        break;
                }
                case REVERTS:{
                        ++m_reverts;
                        break;
                }
                case STALLEDROUNDS:{
                        ++m_turns_stalled;
                        break;
                }
                case DELMSG:{
                        ++m_deleted_msgs;
                        break;
                }
                default:
                        LOG_ERROR("No such logstat type");
                        break;
                }
#endif
        }
};

/**
 * @brief A Core is a node in a Devs simulator. It manages (multiple) atomic models and drives their transitions.
 * A Core only operates on atomic models, the translation from coupled to atomic (leaves) is done by Controller.
 * The Core is responsible for timewarp, messaging, time and scheduling of models.
 */
class Core
{
protected:

	/**
	 * Current simulation time
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
	 * Marks if this core has triggered a terminated functor. This distinction is required
	 * for timewarp, and for the controller to redistribute the current time at wich the
	 * functor triggered as a new termination time (which in turn can be undone ...)
	 */
	std::atomic<bool> m_terminated_functor;
        
protected:        
        /**
         * Stores modelptrs sorted on ascending priority.
         */
        std::vector<t_atomicmodelptr> m_indexed_models;
        
        /**
         * On ordered set of models sorted at imminent time.
         */
        t_scheduler m_heap;

        /**
         * Stores models that will transition in this simulation round.
         * This vector shrinks/expands during the simulation steps.
         */
        std::vector<t_raw_atomic>   m_imminents;

	/**
	 * Total amount of cores.
	 */
	std::size_t m_cores;
	std::size_t m_msgStartCount;
	std::size_t m_msgEndCount;
	std::size_t m_msgCurrentCount;
        
protected:
        /**
         * Messages to process in a current round.
         */
        std::vector<std::vector<t_msgptr>> m_indexed_local_mail;

        /**
         * Stores models that will transition in this simulation round.
         * This vector shrinks/expands during the simulation steps.
         */
        std::vector<t_raw_atomic>   m_externs;

        /**
         * Cached token used to check for messages.
         */
        MessageEntry        m_token;

        /**
         * Temporary buffer used in doOutput().
         */
        std::vector<n_network::t_msgptr> m_mailfrom;

        /**
         * Consecutive simulation rounds where the core had no next event
         * In optimistic used as a tiebreaker to avoid excessive reverts, large values trigger
         * the core registering for exit from the simulation.
         */
        std::size_t m_zombie_rounds;

        /**
         * Return current mail for the model.
         */
        std::vector<t_msgptr>&
        getMail(size_t id);

        /**
         * Check if a model has mail pending.
         */
        bool
        hasMail(size_t id);

	/**
	 * After a simulation step, verify that we need to continue.
	 */
	void
	checkTerminationFunction();
        
        /**
         * Check that the internal state of the core is still sane.
         * @throw std::logic_error
         */
        virtual
        void
        checkInvariants();
    
        /**
         * After a transition (and trace call), the processed messages
         * are no longer needed (except in optimistic). In conservative and single core,
         * the messages are safe to destroy. This method is virtual to allow optimistic to override
         * this behaviour.
         * @param msgs is the vector of msgptrs processed by a single model in a confluent/external transition.
         * @pre msgs.size()>0
         * @post msgs.size()==0
         */
        virtual
        void
        clearProcessedMessages(std::vector<t_msgptr>& msgs);
        
        /**
         * Sort the vector of models by priority, sets indices in models.
         */
        void
        initializeModels();
    
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

        inline bool
        isMessageLocal(t_msgptr msg)const
        {
            return (msg->getDestinationCore()==m_coreid);
        }
        
        virtual
        void
        signalTransition(){;}
        
	/**
	* Store received messages.
	*/
	t_msgscheduler	m_received_messages;

        virtual
	void queuePendingMessage(t_msgptr){assert(false);}
        
        /**
         * Store a generated message between models in this core for local
         * handling. (ie avoid the heap)
         */
        void queueLocalMessage(const t_msgptr& msg);

	/**
	 * Constructor intended for subclass usage only. Same initialisation semantics as default constructor.
	 */
	Core(std::size_t id, std::size_t totalCores);

	/**
	 * Subclass hook. Is called after imminents are collected.
         * Used in DS.
	 */
	virtual
	void
	signalImminent(const std::vector<t_raw_atomic>& ){;}

	/**
	 * In case of a revert, wipe the scheduler clean, inform all models of the changed time and reload the scheduler
	 * with fresh entries.
	 */
	void
	rescheduleAllRevert(const t_timestamp& totime);
        
        /**
         * Wipe the scheduler clear, and ask each model for a new scheduled entry.
         */
        void
        rescheduleAll();

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
	 * @attention : with pool usage Core destructors are no longer able to deallocate.
         * @see shutDown();
	 */
	virtual ~Core();
	
	/**
	 * In optimistic simulation, revert models to earlier stage defined by totime.
	 * @pre totime >= this->getGVT() && totime < this->getTime()
	 */
	virtual
	void revert(const t_timestamp& /*totime*/){assert(false);}

	/**
	 * Add model to this core.
	 * @pre !containsModel(model->getName());
	 */
        virtual
	void addModel(const t_atomicmodelptr& model);
        
        /**
         * Add a model at runtime. Implemented only in DS.
         */
        virtual
	void addModelDS(const t_atomicmodelptr& /*model*/){assert(false);}

	/**
	 * Retrieve model with name from core
	 * @pre model is present in this core.
         * @deprecated
	 */
	t_atomicmodelptr
	getModel(const std::string& name)const;
        
        const t_atomicmodelptr&
        getModel(size_t index)const;

	//deprecated, O(N)
	bool
	containsModel(const std::string& name)const;
        
        /**
         * Check if the model's uuid references a local model.
         * Depends on safety_checks macro
         */
        void
        validateUUID(const n_model::uuid&);

	/**
	 * Live indicates, with the execption of dynstructured, the core is considered
         * in or beyond simulation. 
         * In Single core simulation, live indicates simulation can proceed.
         * In parallel, live indicates whether or not the core has work to do (which in case of revert
         * can go back from dead to live).
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
	 * Run at startup, populate the scheduler with the model's timeadvance() +- elapsed.
	 * @attention : run this once and once only.
	 * @review : this is not part of the constructor since this instance is in a legal state after
	 * the constructor, but needs to receive models piecemeal by the controller.
	 */
	virtual
	void init();

	/**
	 * Run at startup when the thread has already been started.
	 * @attention : run this once and only once. Don't run this on the main thread if the core is multithreaded.
	 */
	virtual
	void initThread();
        
        void
        getImminent(std::vector<t_raw_atomic>& imms);


	/**
	 * Called in case of Dynamic structured Devs.
	 * Stores imminent models into parameter (which is cleared first)
	 * @attention : noop in superclass
	 */
	virtual
	void
	getLastImminents(std::vector<t_raw_atomic>&){
		assert(false && "Not supported in non dynamic structured devs");
	}

	/**
	 * Request a new timeadvance() value from the model, and place an entry (model, ta()) on the scheduler.
	 */
	void
	rescheduleImminent();

	/**
	 * Updates local time. The core time will advance to min(first transition, earliest received unprocessed message).
	 * @attention : changes local state : idle, live, terminated, time. It's possible time does not
	 * advance at all, this is allowed.
	 */
	virtual
	void
	syncTime();

	/**
	 * Run a single DEVS simulation step:
         *      - getMessages() (from remote models, noop in single core)
	 * 	- collect output from imminent models
	 * 	- route messages
	 * 	- transition
	 * 	- trace
	 * 	- reschedule models if needed.
	 * 	- update core time to furthest point possible
         *      - evaluate termination condition. (time or function)
	 * @pre init() has run once, there exists at least 1 model that is scheduled.
	 */
	virtual
	void
	runSmallStep();
        
        /**
	 * Collect output from imminent models, sort them in the mailbag by destination name.
	 * @attention : generated messages (events) are timestamped by the current core time.
	 */
        virtual void
        collectOutput(std::vector<t_raw_atomic>& imminent);

	/**
	 * Hook for subclasses to override. Called whenever a message for the net is found.
	 * @attention assert(false) in single core
	 */
	virtual void sendMessage(t_msgptr)
	{
		assert(false && "A message for a remote core in a single core implementation.");
	}

	/**
	 * Pull messages from network.
         * This is a hook parallel cores override.
	 */
	virtual void getMessages()
	{
		;
	}

	/**
	 * Set current time to new value.
	 * @attention : virtual so that users of Core can call this without locking cost, but a Multicore instance
	 * can invoke locking before calling this method. Equally important, Conservate Core will correct any attempt
	 * to advance time beyond EIT.
	 * @see Multicore, Conservativecore
	 */
	virtual
	void
	setTime(const t_timestamp&);

	/**
	 * Get Current simulation time.
	 * This is a timestamp equivalent to the first model scheduled to transition at the end of a simulation phase (step).
	 * @note The causal field is to be disregarded, it is not relevant here.
	 */
	virtual
	t_timestamp getTime();

	/**
	 * Retrieve GVT. Only makes sense for a multi core.
	 */
	t_timestamp getGVT() const;

	/**
	 * Set current GVT
	 */
	virtual void
	setGVT(const t_timestamp& newgvt);

	/**
	 * Depending on whether a model may transition (imminent), and/or has received messages, transition.
	 */
	void
	transition();

	/**
	 * Debug function : print out the currently scheduled models.
	 */
	void
	printSchedulerState();

	/**
	 * Print all queued messages.
	 * @attention : invokes a full copy of all stored msg ptrs, only for debugging!
	 */
	void
	printPendingMessages();

	/**
	 * Given a set of messages, sort them by model destination.
	 * @attention : for single core no more than a simple sort, for multicore accesses network to push messages not local.
	 */
	virtual void
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

	virtual
	void
	setTerminationTime(t_timestamp endtime);

	virtual
	t_timestamp
	getTerminationTime();

	/**
	 * Return if core has triggered termination functor.
	 * @synchronized
	 */
	bool terminatedByFunctor()const;

	/**
	 * Indicate this instance has terminated by functor.
	 */
	void setTerminatedByFunctor(bool b);

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
        virtual
	void
	removeModel(std::size_t id);
        
        // Move this and use dyn_ptr in DS. works for now.
        virtual
	void
	removeModelDS(std::size_t /*id*/){assert(false);}
        
        // Move this and use dyn_ptr in DS. works for now.
        virtual
        void
        validateModels(){assert(false);}

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
	void receiveMessage(t_msgptr);

	/**
	 * Get the mail with timestamp < nowtime sorted by destination.
	 */
	virtual
	void getPendingMail();

	/**
	 * Record msg as sent.
	 */
	virtual
	void markMessageStored(const t_msgptr&){;}

	/**
	 * Return min of {external received messages} || \infty.
	 */
        virtual
	t_timestamp
	getFirstMessageTime(){return t_timestamp::infinity();}
        
        /**
         * @return Time of first imminent model, or inf.
         */
        t_timestamp
        getFirstImminentTime();


	/**
	 * Mattern's algorithm nrs 1.6/1.7
	 * @attention : only sensible in multicore setting, single core will assert fail.
	 */
	virtual
	void
	receiveControl(const t_controlmsg& /*controlmessage*/, int /*first*/, std::atomic<bool>& /*rungvt*/){
		assert(false);
	}

	/**
	 * Write current Core state to log.
	 */
	void
	logCoreState();

	/**
	 * Return true if the network reports there are still messages going around.
	 * @attention: assert(false) in single core.
	 */
	virtual
	bool
	existTransientMessage();

	/**
	 * @return nr of consecutive simulation steps this core hasn't been able to advance in time (no messages, nothing scheduled).
	 */
	std::size_t
	getZombieRounds(){return m_zombie_rounds;}

        void
        incrementZombieRounds(){++m_zombie_rounds;}
        
        void resetZombieRounds(){m_zombie_rounds=0;}

	virtual
	MessageColor
	getColor();

	virtual
	void
	setColor(MessageColor mc);
        
        /**
         * Invoke at end of simulation, clears remaining messages from buffers (should this be needed).
         */
        virtual
        void
        shutDown(){;}


        friend class n_tools::GVizWriter;
//-------------statistics gathering--------------
protected:
	statistics_collector m_stats;
public:
#ifdef USE_VIZ
        virtual void writeGraph(){
                LOG_DEBUG("Calling gviz for core ::", this->m_coreid);
                n_tools::GVizWriter::getWriter("sim.dot")->writeObject(this);
        }
#endif
        
#ifdef USE_STAT
	virtual void printStats(std::ostream& out = std::cout) const
	{
		m_stats.printStats(out);
	}
#else /* USE_STAT */
	inline void printStats(std::ostream& = std::cout) const
	{ }
#endif
};

typedef std::shared_ptr<Core> t_coreptr;

}

#endif /* SRC_MODEL_CORE_H_ */
