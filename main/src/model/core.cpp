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
#include "tools/vectorscheduler.h"
#include "cereal/types/vector.hpp"

using n_network::MessageEntry;

inline void validateTA(const n_network::t_timestamp& val){
#ifdef SAFETY_CHECKS
	if(isZero(val)) throw std::logic_error("Time Advance value shouldn't be zero.");
#endif
}

n_model::Core::~Core()
{
        delete m_token.getMessage();
}

void
n_model::Core::checkInvariants(){
#ifdef SAFETY_CHECKS
        if(this->m_scheduler->size() > this->m_indexed_models.size()){
                const std::string msg = "Scheduler contains more models than present in core !!";
                LOG_ERROR(msg);
                LOG_FLUSH;
                throw std::logic_error(msg);
        }
#endif
}

n_model::Core::Core():
	Core(0, 1)
{
}

n_model::Core::Core(std::size_t id, std::size_t totalCores)
	:       m_time(0, 0), m_gvt(0, 0), m_coreid(id), m_live(false), m_termtime(t_timestamp::infinity()),
                m_terminated(false), m_termination_function(n_tools::createObject<n_model::TerminationFunctor>()),
                m_idle(false), m_terminated_functor(false), m_cores(totalCores),
                m_token(n_tools::createRawObject<n_network::Message>(uuid(), uuid(), m_time, 0, 0)),m_zombie_rounds(0),
                m_scheduler(new n_tools::VectorScheduler<boost::heap::pairing_heap<ModelEntry>, ModelEntry>),
		m_received_messages(n_tools::SchedulerFactory<MessageEntry>::makeScheduler(n_tools::Storage::FIBONACCI, false, n_tools::KeyStorage::MAP)),
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

	std::vector<ModelEntry> scheduler;
	while (not m_scheduler->empty()) {
		scheduler.push_back(m_scheduler->pop());
	}

	oarchive(m_indexed_models, scheduler);
}

void n_model::Core::load(const std::string& fname)
{
	std::fstream fs (fname, std::fstream::in | std::fstream::binary);
	cereal::BinaryInputArchive iarchive(fs);

	std::vector<ModelEntry> scheduler;

	iarchive(m_indexed_models, scheduler);


	while (not scheduler.empty()) {
		m_scheduler->push_back(scheduler.back());
		scheduler.pop_back();
	}
}

void n_model::Core::addModel(const t_atomicmodelptr& model)
{
        LOG_DEBUG("\tCORE :: ", this->getCoreID(), " Add model called on core::  got model : ", model->getName());
	std::string mname = model->getName();
        this->m_indexed_models.push_back(model);
}


n_model::t_atomicmodelptr n_model::Core::getModel(const std::string& mname)const
{
        LOG_INFO("DEPRECATED!");
        for(const auto& model : m_indexed_models){
                if(model->getName()==mname)
                        return model;
        }
        throw std::out_of_range("No such model in core.");
}

const n_model::t_atomicmodelptr&
n_model::Core::getModel(size_t index)const{
#ifdef SAFETY_CHECKS
        return m_indexed_models.at(index);
#else
        return m_indexed_models[index];
#endif
}

bool n_model::Core::containsModel(const std::string& mname) const
{
        for(const auto& model : m_indexed_models){
                if(model->getName()==mname)
                        return true;
        }
        return false;
}

void n_model::Core::scheduleModel(std::size_t id, t_timestamp t)
{
        checkInvariants();
        
	
        LOG_DEBUG("\tCORE :: ", this->getCoreID(), " got request rescheduling : ", getModel(id)->getName() , "@", t);
        if(isInfinity(t)){
                LOG_INFO("\tCORE :: ", this->getCoreID(), " refusing to schedule ", getModel(id)->getName() , "@", t);
                return;
        }
        const t_atomicmodelptr& model = this->getModel(id);             // Acces ptr here saves ~shared_ptr  release
        const t_timestamp newt(t.getTime(), model->getPriority());
        const ModelEntry entry(id, newt);
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
        this->m_scheduler->hintSize(m_indexed_models.size());
        
        m_imm_ids.reserve(m_indexed_models.size());
        
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

void n_model::Core::collectOutput(std::vector<t_raw_atomic>& imminents)
{
	/**
	 * For each imminent model, collect output.
	 * Then sort that output by destination (for the transition functions)
	 */
	LOG_DEBUG("\tCORE :: ", this->getCoreID(), " Collecting output for ", imminents.size(), " imminents ");
        m_mailfrom.clear();
	for (auto model : imminents) {
		model->doOutput(m_mailfrom);
		LOG_DEBUG("\tCORE :: ", this->getCoreID(), " got ", m_mailfrom.size(), " messages from ", model->getName());

#ifdef SAFETY_CHECKS		
		for (const auto& msg : m_mailfrom) {
                        LOG_DEBUG("\tCORE :: ", this->getCoreID(), " msg uuid info == src::", msg->getSrcUUID(), " dst:: ", msg->getDstUUID());
                        validateUUID(msg->getSrcUUID());
		}
#endif
                
		this->sortMail(m_mailfrom);	// <-- Locked here on msglock. Don't pop_back, single clear = O(1)
                m_mailfrom.clear();
	}
}

std::vector<t_msgptr>& 
n_model::Core::getMail(size_t id){
#ifdef SAFETY_CHECKS
        return m_indexed_local_mail.at(id);
#else
        return m_indexed_local_mail[id];
#endif
}

bool
n_model::Core::hasMail(size_t id){
#ifdef SAFETY_CHECKS
        return m_indexed_local_mail.at(id).size()!=0;
#else
        return m_indexed_local_mail[id].size()!=0;
#endif
}

void n_model::Core::transition()
{
        
	LOG_DEBUG("\tCORE :: ", this->getCoreID(), " Transitioning with ", m_imminents.size(), " imminents, and ");
        
	t_timestamp noncausaltime(this->getTime().getTime(), 0);
	for (auto imminent : m_imminents) {                     
                const size_t modelid = imminent->getLocalID();
		if (!hasMail(modelid)) {			
			assert(imminent->nextType()==INT);
                        LOG_DEBUG("\tCORE :: ", this->getCoreID(), " performing internal transition for model ", imminent->getName());
			imminent->doIntTransition();
			imminent->setTime(noncausaltime);
			this->traceInt(getModel(modelid));
		} else {
                        LOG_DEBUG("\tCORE :: ", this->getCoreID(), " performing confluent transition for model ", imminent->getName());
                        assert(imminent->nextType()==n_model::CONF);
                        imminent->nextType()=n_model::NONE;
			imminent->setTimeElapsed(0);
                        auto& mail = getMail(modelid);
			imminent->doConfTransition(mail);		// Confluent
			imminent->setTime(noncausaltime);
			this->traceConf(getModel(modelid));
                        clearProcessedMessages(mail);        

		}
	}
        for(auto external : m_externs){
                const size_t id = external->getLocalID();
                auto& mail = getMail(id);
		external->setTimeElapsed(noncausaltime.getTime() - external->getTimeLast().getTime());
		external->doExtTransition(mail);
                assert(external->nextType()==EXT);
                external->nextType()=n_model::NONE;
		external->setTime(noncausaltime);
		m_scheduler->erase(ModelEntry(id, t_timestamp(0u,0u)));		// If ta() changed , we need to erase the invalidated entry.
		this->traceExt(getModel(id));
		const t_timestamp queried(external->getTimeNext());		// A previously inactive model can be awoken, make sure we check this.
		if (!isInfinity(queried)) {
			LOG_DEBUG("\tCORE :: ", this->getCoreID(), " Model ", external->getName(),
				" changed ta value to ", queried, " rescheduling.");
			m_imminents.push_back(external);
		}else{
			LOG_DEBUG("\tCORE :: ", this->getCoreID(), " Model ", external->getName(),
				" changed ta value to infinity, no longer scheduling.");
		}
                clearProcessedMessages(mail);
	}
}

void n_model::Core::sortMail(const std::vector<t_msgptr>& messages)
{
	this->lockMessages();
        for(const auto& message : messages){
		LOG_DEBUG("\tCORE :: ", this->getCoreID(), " sorting message ", message->toString());
		if (not this->isMessageLocal(message)) {
                        m_stats.logStat(MSGSENT);
			this->sendMessage(message);	// A noop for single core, multi core handles this.
		} else {
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
n_model::Core::getImminent(std::vector<t_raw_atomic>& imms)
{
	const ModelEntry mark(0, t_timestamp(this->getTime().getTime(), t_timestamp::MAXCAUSAL));
	this->m_scheduler->unschedule_until(m_imm_ids, mark);
	for (const auto& entry : m_imm_ids) {
                auto model = this->getModel(entry.getID()).get();
                model->nextType() |= n_model::INT;
                imms.push_back(model);
        }
        m_imm_ids.clear();
	LOG_DEBUG("\tCORE :: ", this->getCoreID(), " Have ", imms.size(), " imminents @ time " , this->getTime() );
}


void n_model::Core::rescheduleImminent(const std::vector<t_raw_atomic>& oldimms)
{
	LOG_DEBUG("\tCORE :: ", this->getCoreID(), " Rescheduling ", oldimms.size(), " models for next run.");
	for (auto model : oldimms) {
                const t_timestamp next = model->getTimeNext();
                model->nextType()=n_model::NONE;        // Reset transition state
		if (!isInfinity(next)) {
			LOG_DEBUG("\tCORE :: ", this->getCoreID(), " ", model->getName(),
			        " rescheduled @ ", next);
                        //this->m_scheduler->update(ModelEntry(model->getLocalID(), next));
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
	const t_timestamp firstmessagetime(this->getFirstMessageTime());
	LOG_DEBUG("\tCORE :: ", this->getCoreID(), " Candidate for new time is min( ", nextfired, " , ", firstmessagetime , " ) ");
	const t_timestamp newtime = std::min(firstmessagetime, nextfired);
	if (isInfinity(newtime)) {
		LOG_WARNING("\tCORE :: ", this->getCoreID(), " Core has no new time (no msgs, no scheduled models), marking as zombie");
		incrementZombieRounds();
		return;
	}
#ifdef SAFETY_CHECKS
	if (this->getTime() > newtime) {
		LOG_ERROR("\tCORE :: ", this->getCoreID() ," Synctime is setting time backward ?? now:", this->getTime(), " new time :", newtime);
		throw std::runtime_error("Core time going backwards. ");
	}
#endif
	// Here we a valid new time.
	this->setTime(newtime);						// It's possible this stalls time if eit == old time
									// but that is a deadlock, not a zombie state.
	this->resetZombieRounds();

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

void n_model::Core::validateUUID(const n_model::uuid& id)
{
#ifdef SAFETY_CHECKS
        if(! (id.m_core_id == m_coreid && id.m_local_id<m_indexed_models.size() )){
                LOG_ERROR("Core ::", this->getCoreID(), " uuid check failed : ", id, " holding ", m_indexed_models.size());
                LOG_FLUSH;
                throw std::logic_error("UUID validation failed. Check logs.");
        }
#endif  
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

        this->getImminent(m_imminents);
        
        // Dynamic structured needs the list, but best before we add externals to it.
        this->signalImminent(m_imminents);  
        
	// Get all produced messages, and route them.
	this->collectOutput(m_imminents);	

	// Get msg < timenow, sort them for ext/conf.
	this->getPendingMail();			

	// Transition depending on state.
	this->transition();		// NOTE: the scheduler can go empty() here.

	// Finally find out what next firing times are and place models accordingly.
	this->rescheduleImminent(m_imminents);

	// Forward time to next message/firing.
	this->syncTime();				// locked on msgs
        m_imminents.clear();
        m_externs.clear();

	// Do we need to continue ?
	this->checkTerminationFunction();

	// Finally, unlock simulator.
	this->unlockSimulatorStep();
}

void n_model::Core::traceInt(const t_atomicmodelptr& model)
{
#ifndef NO_TRACER
	if (not this->m_tracers) {
		LOG_WARNING("\tCORE :: ", this->getCoreID(), " I have no tracers ?? , tracerset = nullptr.");
	} else {
		this->m_tracers->tracesInternal(model, this->getCoreID());
	}
#endif
}

void n_model::Core::traceExt(const t_atomicmodelptr& model)
{
#ifndef NO_TRACER
	if (not this->m_tracers) {
		LOG_WARNING("\tCORE :: ", this->getCoreID(), " I have no tracers ?? , tracerset = nullptr.");
	} else {
		this->m_tracers->tracesExternal(model, this->getCoreID());
	}
#endif
}

void n_model::Core::traceConf(const t_atomicmodelptr& model)
{
#ifndef NO_TRACER
	if (not this->m_tracers) {
		LOG_WARNING("\tCORE :: ", this->getCoreID(), " I have no tracers ?? , tracerset = nullptr.");
	} else {
		this->m_tracers->tracesConfluent(model, this->getCoreID());
	}
#endif
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
		for (const auto& model : m_indexed_models) {
			if ((*m_termination_function)(model)) {
				LOG_DEBUG("CORE: ", this->getCoreID(), " Termination function evaluated to true for model ", model->getName());
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
        auto iter = m_indexed_models.begin();
        std::advance(iter, lid);
        m_indexed_models.erase(iter);
        LOG_INFO("\tCORE :: ", this->getCoreID(), " removed model : ", name);
        //assert(this->m_scheduler->contains(target) == false && "Removal from scheduler failed !! model still in scheduler");
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

void n_model::Core::clearProcessedMessages(std::vector<t_msgptr>& msgs)
{
#ifdef SAFETY_CHECKS
        if(msgs.size()==0)
                throw std::logic_error("Msgs empty after processing ?");
#endif
        /// Msgs is a vector of processed msgs, stored in m_local_indexed_mail.
        for(auto& ptr : msgs){
                delete ptr;
                m_stats.logStat(DELMSG);
#ifdef SAFETY_CHECKS
                ptr = nullptr;
#endif   
        }
        
        msgs.clear();
}


void n_model::Core::clearModels()
{
	assert(this->isLive() == false && "Clearing models during simulation is not supported.");
	LOG_DEBUG("\tCORE :: ", this->getCoreID(), " removing all models from core.");
        this->m_indexed_local_mail.clear();
        this->m_indexed_models.clear();
	this->m_scheduler->clear();
	this->m_received_messages->clear();
	this->setTime(t_timestamp(0, 0));
	this->m_gvt = t_timestamp(0, 0);
}

void n_model::Core::queuePendingMessage(const t_msgptr& msg)
{
	const MessageEntry entry(msg);
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
        const size_t id = msg->getDstUUID().m_local_id;
        auto model = this->getModel(id).get();
        if(!hasMail(id)){               // If recd msg size==0
                if(model->nextType()==n_model::NONE){   // If INT is set, we get CONF so adding it to imminents risks duplicate transitions
                        m_externs.push_back(model);     // avoid map by checking state.
                }
                model->nextType() |= n_model::EXT;
        }
        getMail(id).push_back(std::move(msg));
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
	for (const auto& model : m_indexed_models) 
		this->scheduleModel(model->getLocalID(), model->getTimeNext());
}


void n_model::Core::receiveMessage(t_msgptr )
{
        assert(false);
}

void n_model::Core::getPendingMail()
{
	/**
	 * Check if we have pending messages with time <= (time=now, caus=oo);
	 * If so, add them to the mailbag
	 */
        this->lockMessages();
        if(m_received_messages->empty()){
                this->unlockMessages();
                return;
        }
        this->unlockMessages();
	const t_timestamp nowtime = makeLatest(m_time);
	std::vector<MessageEntry> messages;
	//std::shared_ptr<n_network::Message> token = n_tools::createObject<n_network::Message>(uuid(), uuid(), nowtime, 0, 0);
	m_token.getMessage()->setTimeStamp(nowtime);

	this->lockMessages();
	this->m_received_messages->unschedule_until(messages, m_token);
	this->unlockMessages();
	for (const auto& entry : messages) 
                queueLocalMessage(entry.getMessage());

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
