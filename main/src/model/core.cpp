/*
 * core.cpp
 *
 *      Author: Ben Cardoen
 */
#include <cassert>
#include <fstream>
#include <stdexcept>
#include "model/core.h"
#include "tools/globallog.h"
#include "tools/objectfactory.h"
#include "cereal/archives/binary.hpp"
#include "cereal/types/string.hpp"
#include "cereal/types/unordered_map.hpp"
#include "cereal/types/vector.hpp"

using n_network::MessageEntry;

inline void validateTA(const n_network::t_timestamp& val){
#ifdef SAFETY_CHECKS
	if(isZero(val)) throw std::logic_error("Time Advance value shouldn't be zero.");
#endif
}

n_model::Core::~Core()
{
	// Make sure we don't keep stale pointers alive
	for (auto& model : m_models) {
		model.second.reset();
	}
	m_models.clear();
	m_received_messages->clear();
}

void
n_model::Core::checkInvariants(){
#ifdef SAFETY_CHECKS
        if(this->m_scheduler->size() > this->m_models.size()){
                const std::string msg = "Scheduler contains more models than present in core !!";
                LOG_ERROR(msg);
                LOG_FLUSH;
                throw std::logic_error(msg);
        }
#endif
}

n_model::Core::Core():
	Core(0)
{
}

n_model::Core::Core(std::size_t id)
	: m_time(0, 0), m_gvt(0, 0), m_coreid(id), m_live(false), m_termtime(t_timestamp::infinity()),
	  m_terminated(false), m_termination_function(n_tools::createObject<n_model::TerminationFunctor>()),
		m_idle(false), m_zombie_rounds(0), m_terminated_functor(false),
		m_scheduler(n_tools::SchedulerFactory<ModelEntry>::makeScheduler(n_tools::Storage::FIBONACCI, false)),
		m_received_messages(n_tools::SchedulerFactory<MessageEntry>::makeScheduler(n_tools::Storage::FIBONACCI, false, true)),
		m_stats(m_coreid)
{
	assert(m_time == t_timestamp(0, 0));
	assert(m_live == false);
}

bool n_model::Core::isMessageLocal(const t_msgptr& msg) const
{
        return (msg->getDestinationCore()==m_coreid);
}

void n_model::Core::save(const std::string& fname)
{
	std::fstream fs (fname, std::fstream::out | std::fstream::trunc | std::fstream::binary);
	cereal::BinaryOutputArchive oarchive(fs);

	std::vector<t_msgptr> messages;
	while (not m_received_messages->empty()) {
		messages.push_back(m_received_messages->pop().getMessage());
	}

	std::vector<ModelEntry> scheduler;
	while (not m_scheduler->empty()) {
		scheduler.push_back(m_scheduler->pop());
	}

	oarchive(m_models, messages, scheduler);
}

void n_model::Core::load(const std::string& fname)
{
	std::fstream fs (fname, std::fstream::in | std::fstream::binary);
	cereal::BinaryInputArchive iarchive(fs);

	std::vector<t_msgptr> messages;
	std::vector<ModelEntry> scheduler;

	iarchive(m_models, messages, scheduler);

	while (not messages.empty()) {
		m_received_messages->push_back(MessageEntry(messages.back()));
		messages.pop_back();
	}

	while (not scheduler.empty()) {
		m_scheduler->push_back(scheduler.back());
		scheduler.pop_back();
	}
}

void n_model::Core::addModel(const t_atomicmodelptr& model)
{
        LOG_DEBUG("\tCORE :: ", this->getCoreID(), " Add model called on core::  got model : ", model->getName());
	std::string mname = model->getName();
	assert(this->m_models.find(mname) == this->m_models.end() && "Model already in core.");
	this->m_models[mname] = model;
        this->m_indexed_models.push_back(model);
}


n_model::t_atomicmodelptr n_model::Core::getModel(const std::string& mname)const
{
        LOG_INFO("DEPRECATED!");
        for(const auto& model : m_indexed_models){
                if(model->getName()==mname)
                        return model;
        }
        assert(false);
}

const n_model::t_atomicmodelptr&
n_model::Core::getModel(size_t index)const{
#ifdef SAFETY_CHECKS
        return m_indexed_models.at(index);
#else
        return m_indexed_model[index];
#endif
}

bool n_model::Core::containsModel(const std::string& mname) const
{
	return (this->m_models.find(mname) != this->m_models.end());
}

void n_model::Core::scheduleModel(std::size_t id, t_timestamp t)
{
        checkInvariants();
        t_atomicmodelptr model = this->getModel(id);
	
        LOG_DEBUG("\tCORE :: ", this->getCoreID(), " got request rescheduling : ", model->getName(), "@", t);
        if(isInfinity(t)){
                LOG_INFO("\tCORE :: ", this->getCoreID(), " refusing to schedule ", model->getName() , "@", t);
                return;
        }
        const t_timestamp newt(t.getTime(), model->getPriority());
        ModelEntry entry(id, newt);
        if (this->m_scheduler->contains(entry)) {
                LOG_INFO("\tCORE :: ", this->getCoreID(), " scheduleModel Tried to schedule a model that is already scheduled: ", model->getName(),
                " at t=", t, " replacing.");
                this->m_scheduler->erase(entry);			// Needed for revert, scheduled entry may be wrong.
        }
        this->m_scheduler->push_back(entry);

        checkInvariants();
}

void n_model::Core::init()
{
	if (this->m_scheduler->size() != 0) {
		LOG_ERROR("\tCORE :: ", this->getCoreID(),
		" scheduler is not empty on call to init(), cowardly refusing to corrupt state any further.");
		return;
	}
        this->initializeModels();
        
	for (const auto& model : this->m_indexed_models) {
		const t_timestamp modelTime(this->getTime().getTime() - model->getTimeElapsed().getTime(),0);
		model->setTime(modelTime);	// DO NOT use priority, model does this already
		const t_timestamp model_scheduled_time = model->getTimeNext(); // model.second->timeAdvance();
		this->scheduleModel(model->getLocalID(), model_scheduled_time);
		m_tracers->tracesInit(model, t_timestamp(0, model->getPriority()));
	}
}

void n_model::Core::initializeModels()
{
        LOG_DEBUG("\tCORE :: ", this->getCoreID(), " initializing models ");
        auto cmp_prior = [=](const t_atomicmodelptr& left, const t_atomicmodelptr& right)->bool{
                return left->getPriority() < right->getPriority();
        };
        std::sort(m_indexed_models.begin(), m_indexed_models.end(), cmp_prior);
        
        assert(m_indexed_models.size()==m_models.size());
        
        m_indexed_local_mail.resize(m_indexed_models.size());
        
        for(size_t index = 0; index<m_indexed_models.size(); ++index){
                const t_atomicmodelptr& model = m_indexed_models[index];
                model->initUUID(this->getCoreID(), index);
                LOG_DEBUG("\tCORE :: ", this->getCoreID(), " uuid of ", model->getName() , " is ", model->getUUID().m_core_id, " local ", model->getUUID().m_local_id);
        }
}
       


void n_model::Core::initExistingSimulation(const t_timestamp& loaddate){
        assert(false);
	if (this->m_scheduler->size() != 0) {
		LOG_ERROR("\tCORE :: ", this->getCoreID(),
		" scheduler is not empty on call to initExistingSimulation(), cowardly refusing to corrupt state any further.");
		return;
	}
        
	for (const auto& model : this->m_indexed_models) {
		LOG_DEBUG("\tCORE :: ", this->getCoreID(), " has ", model->getName());
	}
	LOG_DEBUG("\tCORE :: ", this->getCoreID(), " Reinitializing with loaddate ", loaddate );
	this->m_gvt = loaddate;
	this->m_time = loaddate;
	for (const auto& model : this->m_indexed_models) {
		LOG_INFO("Model ", model->getName(), " TImenext = ", model->getTimeNext(), " loaddate ", loaddate);
		t_timestamp model_scheduled_time(model->getTimeNext().getTime(), 0); // model.second->timeAdvance();
		this->scheduleModel(model->getLocalID(), model_scheduled_time);
		m_tracers->tracesInit(model, t_timestamp(0, model->getPriority()));
	}
}

void n_model::Core::collectOutput(std::vector<t_atomicmodelptr>& imminents)
{
	/**
	 * For each imminent model, collect output.
	 * Then sort that output by destination (for the transition functions)
	 */
	LOG_DEBUG("\tCORE :: ", this->getCoreID(), " Collecting output for ", imminents.size(), " imminents ");
	std::vector<n_network::t_msgptr> mailfrom;
	for (const auto& model : imminents) {
		model->doOutput(mailfrom);
		LOG_DEBUG("\tCORE :: ", this->getCoreID(), " got ", mailfrom.size(), " messages from ", model->getName());
		
		for (const auto& msg : mailfrom) {
                        LOG_DEBUG("\tCORE :: ", this->getCoreID(), " msg uuid info == src::", msg->getSrcUUID(), " dst:: ", msg->getDstUUID());
                        validateUUID(msg->getSrcUUID());
			paintMessage(msg);
			msg->setTimeStamp(this->getTime());
		}
		this->sortMail(mailfrom);	// <-- Locked here on msglock
		mailfrom.clear();		//clear the vector of messages
	}
}

void n_model::Core::transition(std::vector<t_atomicmodelptr>& imminents,
        std::unordered_map<std::string, std::vector<t_msgptr>>& mail)
{
	// Imminents : need at least internal transition
	// Mail : models with pending messages (ext or confluent)
	LOG_DEBUG("\tCORE :: ", this->getCoreID(), " Transitioning with ", imminents.size(), " imminents, and ",
	        mail.size(), " models to deliver mail to.");
        
	t_timestamp noncausaltime(this->getTime().getTime(), 0);
	for (const auto& imminent : imminents) {
		auto found = mail.find(imminent->getName());
		if (found == mail.end()) {				// Internal
			
                        LOG_DEBUG("\tCORE :: ", this->getCoreID(), " performing internal transition for model ", imminent->getName());
			imminent->doIntTransition();
			imminent->setTime(noncausaltime);
			this->traceInt(imminent);
		} else {
                        LOG_DEBUG("\tCORE :: ", this->getCoreID(), " performing confluent transition for model ", imminent->getName());
			imminent->setTimeElapsed(0);
			imminent->doConfTransition(found->second);		// Confluent
			imminent->setTime(noncausaltime);
			this->traceConf(imminent);
			std::size_t erased = mail.erase(imminent->getName()); 	// Erase so we don't need to double check in the next for loop.
			assert(erased != 0 && "Broken logic in collected output");    
		}
	}

	for (const auto& remaining : mail) {				// External
		LOG_DEBUG("\tCORE :: ", this->getCoreID(), " delivering " , remaining.second.size(), " messages to ", remaining.first);
		const t_atomicmodelptr& model = this->m_models[remaining.first];
		model->setTimeElapsed(noncausaltime.getTime() - model->getTimeLast().getTime());
		model->doExtTransition(remaining.second);
		model->setTime(noncausaltime);
		
		m_scheduler->erase(ModelEntry(model->getUUID().m_local_id, this->getTime()));		// If ta() changed , we need to erase the invalidated entry.
		this->traceExt(model);
		t_timestamp queried = model->timeAdvance();		// A previously inactive model can be awoken, make sure we check this.
                validateTA(queried);
		if (!isInfinity(queried)) {
			LOG_DEBUG("\tCORE :: ", this->getCoreID(), " Model ", model->getName(),
				" changed ta value to ", queried, " rescheduling.");
			imminents.push_back(model);
		}else{
			LOG_DEBUG("\tCORE :: ", this->getCoreID(), " Model ", model->getName(),
				" changed ta value to infinity, no longer scheduling.");
		}
	}
}

void n_model::Core::sortMail(const std::vector<t_msgptr>& messages)
{
	this->lockMessages();
	for (const auto & message : messages) {
		LOG_DEBUG("\tCORE :: ", this->getCoreID(), " sorting message ", message->toString());
		if (not this->isMessageLocal(message)) {
                        m_stats.logStat(MSGSENT);
			this->sendMessage(message);	// A noop for single core, multi core handles this.
		} else {
                        // Message is generated at our time, so skip heap.
			this->queueLocalMessage(message);
		}
	}
	this->unlockMessages();
}

void n_model::Core::printSchedulerState()
{
	this->m_scheduler->printScheduler();
}

void
n_model::Core::getImminent(std::vector<t_atomicmodelptr>& imms)
{
	std::vector<ModelEntry> bag;
	const ModelEntry mark(0, t_timestamp(this->getTime().getTime(), t_timestamp::MAXCAUSAL));
	this->m_scheduler->unschedule_until(bag, mark);
	for (const auto& entry : bag) 
                imms.push_back(this->getModel(entry.getID()));
	LOG_DEBUG("\tCORE :: ", this->getCoreID(), " Have ", imms.size(), " imminents @ time " , this->getTime() );
}


void n_model::Core::rescheduleImminent(const std::vector<t_atomicmodelptr>& oldimms)
{
	LOG_DEBUG("\tCORE :: ", this->getCoreID(), " Rescheduling ", oldimms.size(), " models for next run.");
	for (const auto& model : oldimms) {
		t_timestamp ta = model->timeAdvance();
		//check the time advance value
		validateTA(ta);
		if (!isInfinity(ta)) {
			t_timestamp next = ta + this->m_time;
			LOG_DEBUG("\tCORE :: ", this->getCoreID(), " ", model->getName(), " timeadv = ", ta,
			        " rescheduled @ ", next);
			this->scheduleModel(model->getLocalID(), next);		// DO NOT add priority, scheduleModel handles this.
		} else {
			LOG_INFO("\tCORE :: ", this->getCoreID() , " " , model->getName(), " is no longer scheduled (infinity) ");
		}
	}
}

void n_model::Core::syncTime()
{
	/**
	 * We need to advance time from now [x,y] to  min(first message, first scheduled transition).
	 * Most of this code are safety checks.
	 * Locking : we're in SimulatorLock, and request/release Messagelock.
	 */
	t_timestamp nextfired = t_timestamp::infinity();
	if (not this->m_scheduler->empty()) {
		nextfired = this->m_scheduler->top().getTime();
	}
	t_timestamp firstmessagetime = this->getFirstMessageTime();
	LOG_DEBUG("\tCORE :: ", this->getCoreID(), " Candidate for new time is min( ", nextfired, " , ", firstmessagetime , " ) ");
	t_timestamp newtime = std::min(firstmessagetime, nextfired);
	if (isInfinity(newtime)) {
		LOG_WARNING("\tCORE :: ", this->getCoreID(), " Core has no new time (no msgs, no scheduled models), marking as zombie");
		this->m_zombie_rounds.fetch_add(1);
		return;
	}
	if (this->getTime() > newtime) {
		LOG_ERROR("\tCORE :: ", this->getCoreID() ," Synctime is setting time backward ?? now:", this->getTime(), " new time :", newtime);
		throw std::runtime_error("Core time going backwards. ");
	}
	// Here we a valid new time.
	this->setTime(newtime);						// It's possible this stalls time if eit == old time
									// but that is a deadlock, not a zombie state.
	this->m_zombie_rounds.store(0);					// reset zombie state.

	if (this->getTime() >= this->getTerminationTime()) {
		LOG_DEBUG("\tCORE :: ",this->getCoreID() ," Reached termination time :: now: ", this->getTime(), " >= ", this->getTerminationTime());
		this->setLive(false);
		this->setIdle(true);
	}
}

n_network::t_timestamp n_model::Core::getTime()
{
	return this->m_time;
}

n_network::t_timestamp n_model::Core::getGVT() const
{
	return this->m_gvt;
}

bool n_model::Core::isLive() const
{
	return m_live;
}

bool n_model::Core::isIdle() const
{
	return m_idle;
}

void n_model::Core::setIdle(bool idlestat)
{
	LOG_DEBUG("\tCORE :: ", this->getCoreID(), " setting state to idle=", idlestat);
	m_idle.store(idlestat);
}

void n_model::Core::setLive(bool b)
{
	m_live.store(b);
}

std::size_t n_model::Core::getCoreID() const
{
	return m_coreid;
}

void n_model::Core::clearWaitingOutput(){
        // TODO
        m_mailbag.clear();
}

void n_model::Core::validateUUID(const n_model::uuid& id)
{
        if(! (id.m_core_id == m_coreid && id.m_local_id<m_indexed_models.size() )){
                LOG_ERROR("Core ::", this->getCoreID(), " uuid check failed : ", id, " holding ", m_indexed_models.size());
                throw std::logic_error("UUID validation failed. Check logs.");
        }
        
}


void n_model::Core::runSmallStep()
{
	// Lock simulator to allow setGVT/Revert to clear things up.
	this->lockSimulatorStep();
        
        m_stats.logStat(TURNS);

	// Noop in single core. Pull messages from network, sort them.
	// This step can trigger a revert, which is why its before getImminent
	this->getMessages();	// locked on msgs

	if (this->isIdle()) {// If we're done, but the others aren't, check if we have reverted. If not, skip rest of work.
		LOG_DEBUG("\tCORE :: ", this->getCoreID(),
		        " skipping small Step, we're idle and got no messages.");
		this->unlockSimulatorStep();
		return;
	}

	// Query imminent models (who are about to fire transition)
        std::vector<t_atomicmodelptr> imms;
        this->getImminent(imms);
        //Translate for now

	// Get all produced messages, and route them.
	this->collectOutput(imms);			// locked on msgs
        // ^^ change concurrently with conservativecore's collectOutput

	// Give DynStructured Devs a chance to store imminent models.
	this->signalImminent(imms);

	// Get msg < timenow, sort them for ext/conf.
        
	this->getPendingMail(m_mailbag);			// locked on msgs

	// Transition depending on state.
	this->transition(imms, m_mailbag);		// NOTE: the scheduler can go empty() here.

	// Finally find out what next firing times are and place models accordingly.
	this->rescheduleImminent(imms);

	// Forward time to next message/firing.
	this->syncTime();				// locked on msgs

	// Do we need to continue ?
	this->checkTerminationFunction();
        
        clearWaitingOutput(); // Zero mailbag.

	// Finally, unlock simulator.
	this->unlockSimulatorStep();
}

void n_model::Core::traceInt(const t_atomicmodelptr& model)
{
	if (not this->m_tracers) {
		LOG_WARNING("\tCORE :: ", this->getCoreID(), " I have no tracers ?? , tracerset = nullptr.");
	} else {
		this->m_tracers->tracesInternal(model, this->getCoreID());
	}
}

void n_model::Core::traceExt(const t_atomicmodelptr& model)
{
	if (not this->m_tracers) {
		LOG_WARNING("\tCORE :: ", this->getCoreID(), " I have no tracers ?? , tracerset = nullptr.");
	} else {
		this->m_tracers->tracesExternal(model, this->getCoreID());
	}
}

void n_model::Core::traceConf(const t_atomicmodelptr& model)
{
	if (not this->m_tracers) {
		LOG_WARNING("\tCORE :: ", this->getCoreID(), " I have no tracers ?? , tracerset = nullptr.");
	} else {
		this->m_tracers->tracesConfluent(model, this->getCoreID());
	}
}

void n_model::Core::setTerminationTime(t_timestamp endtime)
{
	this->m_termtime = endtime;
	// If we're ahead of this time when we receive it (caused by async functor termination),
	// we're fine to just stop. (@see Yentl's paper.)
}

n_network::t_timestamp n_model::Core::getTerminationTime()
{
	return m_termtime;
}

void n_model::Core::setTerminationFunction(const t_terminationfunctor& fun)
{
	this->m_termination_function = fun;
}

void n_model::Core::checkTerminationFunction()
{
	if (m_termination_function) {
		LOG_DEBUG("\tCORE :: ", this->getCoreID(), " Checking termination function.");
		for (const auto& model : m_models) {
			if ((*m_termination_function)(model.second)) {
				LOG_DEBUG("CORE: ", this->getCoreID(), " Termination function evaluated to true for model ", model.first);
				this->setLive(false);
				this->setIdle(true);
				this->setTerminationTime(this->getTime());
				this->m_terminated_functor.store(true);
				return;
			}
		}
	} else {
		LOG_WARNING("\tCORE :: ", this->getCoreID(), " Termination functor == nullptr, not evaluating.");
	}
}

void n_model::Core::removeModel(const std::string& name)
{
        LOG_INFO("\tCORE :: ", this->getCoreID(), " got request to remove model : ", name);
        
        const t_atomicmodelptr& model = this->getModel(name);
                
        const size_t lid = model->getUUID().m_local_id;
        m_models.erase(name);

        auto iter = m_indexed_models.begin();
        std::advance(iter, lid);
        m_indexed_models.erase(iter);
        
        const ModelEntry target(lid, 0);
        this->m_scheduler->erase(target);       // irrelevant if it exist or not.

        LOG_INFO("\tCORE :: ", this->getCoreID(), " removed model : ", name);
        assert(m_models.size()==m_indexed_models.size());

        assert(this->m_scheduler->contains(target) == false && "Removal from scheduler failed !! model still in scheduler");
}

void n_model::Core::setTime(const t_timestamp& t)
{
	LOG_DEBUG("\tCORE :: ", this->getCoreID(), " setting time from ", m_time, " to ", t);
	m_time = t;
}

void n_model::Core::setTracers(n_tracers::t_tracersetptr ptr)
{
	assert(this->isLive() == false && "Can't change the tracers of a live core.");
	m_tracers = ptr;
}

void n_model::Core::signalTracersFlush() const
{
	t_timestamp marktime(this->m_time.getTime(), std::numeric_limits<t_timestamp::t_causal>::max());
	LOG_DEBUG("\tCORE :: ", this->getCoreID(), " asking tracers to write output up to ", marktime);
	n_tracers::traceUntil(marktime);
}

void n_model::Core::clearModels()
{
	assert(this->isLive() == false && "Clearing models during simulation is not supported.");
	LOG_DEBUG("\tCORE :: ", this->getCoreID(), " removing all models from core.");
	this->m_models.clear();
        this->m_indexed_local_mail.clear();
        this->m_indexed_models.clear();
        this->m_mailbag.clear();
	this->m_scheduler->clear();
	this->m_received_messages->clear();
	this->setTime(t_timestamp(0, 0));
	this->m_gvt = t_timestamp(0, 0);
}

void n_model::Core::queuePendingMessage(const t_msgptr& msg)
{
	MessageEntry entry(msg);
	if(not this->m_received_messages->contains(entry)){
		this->m_received_messages->push_back(entry);
	}else{
		LOG_WARNING("\tCORE :: ", this->getCoreID(), " QPending messages already contains msg, overwriting ", msg->toString());
		this->m_received_messages->erase(entry);
		this->m_received_messages->push_back(entry);
	}
}

void n_model::Core::queueLocalMessage(const t_msgptr& msg)
{
        LOG_DEBUG("\tCORE :: ", this->getCoreID(), " queueing local message (skip heap) ", msg->toString());
        const auto& destname = msg->getDestinationModel();
        if(m_mailbag.find(destname)==m_mailbag.end()){
                m_mailbag[destname]=std::vector<t_msgptr>();
        }
        m_mailbag[destname].push_back(msg);
}


void n_model::Core::rescheduleAllRevert(const t_timestamp& totime)
{
	this->m_scheduler->clear();
	assert(m_scheduler->empty());
	for (const auto& model : m_indexed_models) {
		t_timestamp modellast = model->revert(totime);
		// Bug lived here : Do not set time on model.
                this->scheduleModel(model->getLocalID(), modellast);
	}
}

void n_model::Core::rescheduleAll()
{
        this->m_scheduler->clear();
	assert(m_scheduler->empty());
	for (const auto& model : m_indexed_models) {
                t_timestamp nval = model->getTimeNext();
		this->scheduleModel(model->getLocalID(), nval);
	}
}


void n_model::Core::receiveMessage(const t_msgptr& )
{
;
}

void n_model::Core::getPendingMail(std::unordered_map<std::string, std::vector<t_msgptr>>& mailbag)
{
	/**
	 * Check if we have pending messages with time <= (time=now, caus=oo);
	 * If so, add them to the mailbag
	 */
	t_timestamp nowtime = makeLatest(this->getTime());
	std::vector<MessageEntry> messages;
	std::shared_ptr<n_network::Message> token = n_tools::createObject<n_network::Message>("", nowtime, "", "", "");
	MessageEntry tokentime(token);

	this->lockMessages();
	this->m_received_messages->unschedule_until(messages, tokentime);
	this->unlockMessages();

	for (const auto& entry : messages) {
		std::string modelname = entry.getMessage()->getDestinationModel();
		if (mailbag.find(modelname) == mailbag.end()) {
				mailbag[modelname] = std::vector<t_msgptr>();	// Only make them if we have mail.
		}
		mailbag[modelname].push_back(entry.getMessage());
	}
}

t_timestamp n_model::Core::getFirstMessageTime()
{
        /**
         * Only look at remote messages (opt& cons) for this value, 
         * current messages are (should be processed), so irrelevant.
         */
	t_timestamp mintime = t_timestamp::infinity();
	this->lockMessages();
	if(not this->m_received_messages->empty()){
		mintime = this->m_received_messages->top().getMessage()->getTimeStamp();
	}
	this->unlockMessages();
	LOG_DEBUG("\tCORE :: ", this->getCoreID(), " first message time == ", mintime);
	return mintime;
}

void n_model::Core::setGVT(const t_timestamp& newgvt)
{
	if (isInfinity(newgvt)) {
		LOG_WARNING("\tCORE :: ", this->getCoreID(), " received request to set gvt to infinity, ignoring.");
		return;
	}
	if (newgvt.getTime() < this->getGVT().getTime()) {
		LOG_WARNING("\tCORE :: ", this->getCoreID(), " received request to set gvt to ", newgvt, " < ",
		        this->getGVT(), " ignoring ");
		return;
	}
	LOG_INFO("\tCORE :: ", this->getCoreID(), " Setting gvt from ::", this->getGVT(), " to ", newgvt.getTime());
	this->m_gvt = t_timestamp(newgvt.getTime(), 0);
}

void n_model::Core::printPendingMessages()
{
	this->m_received_messages->printScheduler();
}

void n_model::Core::revertTracerUntil(const t_timestamp& totime)
{
	n_tracers::revertTo(totime, this->getCoreID());
}

void n_model::Core::logCoreState()
{
	LOG_DEBUG("\tCORE :: ", this->getCoreID(), " time= ", this->getTime(), " gvt=", this->getGVT(), " live=",
	        this->isLive(), " idle=", this->isIdle());
}


bool
n_model::Core::existTransientMessage(){
	LOG_ERROR("\tCORE :: ", this->getCoreID(), " existTransientMessage called on single core.");
	throw std::runtime_error("You invoked existTransientMessage on a single core implementation (which has no network)!)");
}

std::size_t
n_model::Core::getZombieRounds(){
	return m_zombie_rounds;
}

bool
n_model::Core::terminatedByFunctor()const{
	return m_terminated_functor;
}

void
n_model::Core::setTerminatedByFunctor(bool b){
	this->m_terminated_functor.store(b);
}

MessageColor
n_model::Core::getColor(){
	assert(false);
	return MessageColor::WHITE;
}

void
n_model::Core::setColor(MessageColor){
	assert(false);
}

void n_model::Core::serialize(n_serialization::t_oarchive& archive) {
	archive(m_time, m_gvt);
}

void n_model::Core::serialize(n_serialization::t_iarchive& archive) {
	archive(m_time, m_gvt);
}

void n_model::Core::load_and_construct(n_serialization::t_iarchive&, cereal::construct<n_model::Core>& construct )
{
	construct();
}
