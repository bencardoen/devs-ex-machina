/*
 * This file is part of the DEVS Ex Machina project.
 * Copyright 2014 - 2015 University of Antwerp 
 * https://www.uantwerpen.be/en/
 * Licensed under the EUPL V.1.1
 * A full copy of the license is in COPYING.txt, or can be found at 
 * https://joinup.ec.europa.eu/community/eupl/og_page/eupl 
 *      Author: Stijn Manhaeve, Ben Cardoen, Tim Tuijn
 */
#include <cassert>
#include <stdexcept>
#include <algorithm>
#include "model/core.h"
#include "tools/globallog.h"
#include "tools/objectfactory.h"
#include "scheduler/vectorscheduler.h"
#include "tools/heap.h"

using n_network::MessageEntry;
using n_network::t_timestamp;
using namespace n_network;

inline void validateTA(const n_network::t_timestamp& val){
#ifdef SAFETY_CHECKS
	if(isZero(val)) throw std::logic_error("Time Advance value shouldn't be zero.");
#endif
}

n_model::Core::~Core()
{
        n_tools::takeBack(m_token.getMessage());
}

void
n_model::Core::checkInvariants(){
#ifdef SAFETY_CHECKS
        if(this->m_heap.indexSize() != this->m_indexed_models.size()){
                printSchedulerState();
                const std::string msg = "Scheduler contains less models than present in core !!";
                LOG_ERROR(msg);
                LOG_FLUSH;
                throw std::logic_error(msg);
        }
        LOG_DEBUG("\tCORE :: ", this->getCoreID(), " testing invariants.");
        m_heap.testInvariant();
#endif
}

n_model::Core::Core():
	Core(0, 1)
{
}

n_model::Core::Core(std::size_t id, std::size_t totalCores)
	:       m_time(0, 0), m_gvt(0, 0), m_coreid(id), m_live(false), m_termtime(t_timestamp::infinity()),
                m_terminated(false),
                m_terminated_functor(false), m_cores(totalCores), m_msgStartCount(id*(std::numeric_limits<std::size_t>::max()/totalCores)),
                m_msgEndCount((id+1)*(std::numeric_limits<std::size_t>::max()/totalCores)-1), m_msgCurrentCount(m_msgStartCount),
                m_token(n_tools::createRawObject<n_network::Message>(uuid(0,0), uuid(0,0), m_time, 0, 0)),m_zombie_rounds(0),
		m_received_messages(n_scheduler::SchedulerFactory<MessageEntry>::makeScheduler(n_scheduler::Storage::FIBONACCI, false, n_scheduler::KeyStorage::NONE)),
		m_stats(m_coreid)
                
{
	assert(m_time == t_timestamp(0, 0));
	assert(m_live == false);
}

void n_model::Core::addModel(const t_atomicmodelptr& model)
{
        LOG_DEBUG("\tCORE :: ", this->getCoreID(), " Add model called on core::  got model : ", model->getName());
        model->initUUID(getCoreID(), m_indexed_models.size());
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

void n_model::Core::init()
{
	if (this->m_heap.size() != 0) {
		LOG_ERROR("\tCORE :: ", this->getCoreID(),
		" scheduler is not empty on call to init(), cowardly refusing to corrupt state any further.");
		return;
	}
        this->initializeModels();

        m_heap.reserve(m_indexed_models.size());
        
	for (auto& model : this->m_indexed_models) {
		const t_timestamp modelTime(this->getTime().getTime() - model->getTimeElapsed().getTime(),0);
		model->setTime(modelTime);	// DO NOT use priority, model does this already
		m_heap.push_back(model.get());
                if(m_tracers){
                        m_tracers->tracesInit(model, t_timestamp(0, model->getPriority()));
                }
                else{
                        LOG_WARNING("No Tracers set on core!", this->getCoreID());
                }
	}
	rescheduleAll();
}

void n_model::Core::initThread()
{}

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

void n_model::Core::collectOutput(std::vector<t_raw_atomic>& imminents)
{
	LOG_DEBUG("\tCORE :: ", this->getCoreID(), " Collecting output for ", imminents.size(), " imminents ");
        m_mailfrom.clear();
	for (auto model : imminents) {
		model->doOutput(m_mailfrom);
		LOG_DEBUG("\tCORE :: ", this->getCoreID(), " got ", m_mailfrom.size(), " messages from ", model->getName());

#ifdef SAFETY_CHECKS		
		for (t_msgptr msg : m_mailfrom) {
                        validateUUID(uuid(msg->getSourceCore(),msg->getSourceModel()));
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
        
	LOG_DEBUG("\tCORE :: ", this->getCoreID(),"@ ", this->getTime(), " Transitioning with ", m_imminents.size(), " imminents");
#ifdef SAFETY_CHECKS
        std::set<t_raw_atomic> transitioning;
        for(auto i : m_imminents){
                if(! transitioning.insert(i).second){
                        LOG_ERROR("Duplicate entry in imminents :: ", i->getName());
                        throw std::logic_error("Duplicate entry in imminents.");
                }
        }
        for(auto e : m_externs){
                if(! transitioning.insert(e).second){
                        LOG_ERROR("Duplicate entry in imminents :: ", e->getName());
                        throw std::logic_error("Duplicate entry in externs.");
                }
        }        
#endif        
	t_timestamp noncausaltime(this->getTime().getTime(), 0);

	const std::size_t k = m_imminents.size() + m_externs.size();
#ifndef PDEVS 	
        m_heap.signalUpdateSize(k);
	LOG_DEBUG("\tCORE :: ", this->getCoreID(), "calculating whether we should reschedule one by one: k=", k, " N=", m_indexed_models.size(), " oneByOne=", m_heap.doSingleUpdate());
#endif
     
#ifdef PDEVS        
#pragma omp parallel for num_threads( PDEVS_THREADS )
        for (std::vector<t_raw_atomic>::iterator it = m_imminents.begin(); it < m_imminents.end(); ++it){
                auto imminent = *it;
#else
	for (t_raw_atomic imminent : m_imminents) {
#endif
                const size_t modelid = imminent->getLocalID();
                LOG_DEBUG("\tCORE :: ", this->getCoreID(), " imminent nextType() = ", int(imminent->nextType()));
		if (!hasMail(modelid)) {
                        LOG_DEBUG("\tCORE :: ", this->getCoreID(), " performing internal transition for model ", imminent->getName());
			assert(imminent->nextType()==AtomicModel_impl::INT);
                        imminent->markNone();
                        imminent->setTimeElapsed(imminent->getTimeNext() - imminent->getTimeLast());
			imminent->doIntTransition();
			imminent->setTime(noncausaltime);
			this->traceInt(getModel(modelid));
		} else {
                        LOG_DEBUG("\tCORE :: ", this->getCoreID(), " performing confluent transition for model ", imminent->getName());
                        assert(imminent->nextType() == AtomicModel_impl::CONF);
                        imminent->markNone();
                        imminent->setTimeElapsed(imminent->getTimeNext() - imminent->getTimeLast());
                        std::vector<t_msgptr>& mail = getMail(modelid);
			imminent->doConfTransition(mail);
			imminent->setTime(noncausaltime);
			this->traceConf(getModel(modelid));
			clearProcessedMessages(mail);
                        assert(!hasMail(modelid) && "After confluent transition, model may no longer have pending mail.");
		}
                imminent->clearSentMessages();
                LOG_DEBUG("\tCORE :: ", this->getCoreID(), " fixing scheduler heap.");
#ifndef PDEVS                
		if(m_heap.doSingleUpdate())
			m_heap.update(modelid);
#endif
                LOG_DEBUG("\tCORE :: ", this->getCoreID(), " result.");
	}
                
        LOG_DEBUG("\tCORE :: ", this->getCoreID(), " Transitioning with ", m_externs.size(), " externs");
#ifdef PDEVS
#pragma omp parallel for num_threads(4)
        for (std::vector<t_raw_atomic>::iterator it = m_externs.begin(); it < m_externs.end(); ++it){
                auto external = *it;
#else
        for(auto external : m_externs){
#endif
                LOG_DEBUG("\tCORE :: ", this->getCoreID(), " performing external transition for model ", external->getName());
                const size_t id = external->getLocalID();
                auto& mail = getMail(id);
                LOG_DEBUG("\tCORE :: ", this->getCoreID(), " performing external transition for model ", external->getName());
		external->setTimeElapsed(noncausaltime.getTime() - external->getTimeLast().getTime());
		external->doExtTransition(mail);
                assert(external->nextType() == AtomicModel_impl::EXT);
                external->markNone();
		external->setTime(noncausaltime);
		this->traceExt(getModel(id));

                clearProcessedMessages(mail);
                LOG_DEBUG("\tCORE :: ", this->getCoreID(), " fixing scheduler heap.");
#ifndef PDEVS
		if(m_heap.doSingleUpdate())
			m_heap.update(id);
#endif
                LOG_DEBUG("\tCORE :: ", this->getCoreID(), " result.");                
		assert(!hasMail(id) && "After external transition, model may no longer have pending mail.");
	}
        
        if(k)
                signalTransition();
}

void n_model::Core::sortMail(const std::vector<t_msgptr>& messages)
{
        for (const auto& message : messages) {
                message->setCausality(m_msgCurrentCount);
                m_msgCurrentCount = m_msgCurrentCount==m_msgEndCount? m_msgStartCount: (m_msgCurrentCount+1);
                LOG_DEBUG("\tCORE :: ", this->getCoreID(), " sorting message ", message->toString());
                this->queueLocalMessage(message);
        }
}

void n_model::Core::printSchedulerState()
{
#if (LOG_LEVEL != 0)
	LOG_DEBUG("Core :: ", getCoreID(), " Scheduler state at time ", getTime());
	LOG_DEBUG("Core :: ", getCoreID(), " indexed models size: ", m_indexed_models.size());
	LOG_DEBUG("Core :: ", getCoreID(), "    heap models size: ", m_heap.size());
	m_heap.printScheduler("Core :: ", getCoreID());
#endif
}

void
n_model::Core::getImminent(std::vector<t_raw_atomic>& imms)
{
	//assumes that m_heap_models is a min heap
	checkInvariants();
	LOG_DEBUG("Core :: ", getCoreID(), " getting imminents.");
	const n_network::t_timestamp::t_time mark = this->getTime().getTime();
	LOG_DEBUG("Core :: ", getCoreID(), "   -> mark: ", mark);
	m_heap.findUntil(imms, mark);
	LOG_DEBUG("\tCORE :: ", this->getCoreID(), " Have ", imms.size(), " imminents @ time " , this->getTime() );
}


void n_model::Core::rescheduleImminent()
{
	LOG_DEBUG("\tCORE :: ", this->getCoreID(), " Rescheduling ", m_imminents.size() + m_externs.size(), " models for next run.");
#ifndef PDEVS
	if(!m_heap.doSingleUpdate()){
#endif
		m_heap.updateAll();
#ifndef PDEVS
	}
#endif
	printSchedulerState();
}


t_timestamp 
n_model::Core::getFirstImminentTime()
{
        t_timestamp nextimm = m_heap.topTime();
        LOG_DEBUG("\tCORE :: ", this->getCoreID(), " @ current time ::  ", this->getTime(), " first imm == ", nextimm);
        return nextimm;
}


void n_model::Core::syncTime()
{
	t_timestamp nextfired = this->getFirstImminentTime();
	const t_timestamp firstmessagetime(this->getFirstMessageTime());
        
	LOG_DEBUG("\tCORE :: ", this->getCoreID(), " Candidate for new time is min( ", nextfired, " , ", firstmessagetime , " ) ");
	const t_timestamp newtime = std::min(firstmessagetime, nextfired);
	if (isInfinity(newtime)) {
		LOG_WARNING("\tCORE :: ", this->getCoreID(), " Core has no new time (no msgs, no scheduled models), marking as zombie");
		incrementZombieRounds();
		if(m_zombie_rounds == 1){
                        LOG_WARNING("\tCORE :: ", this->getCoreID(), " Setting time to last+1 to avoid unnecessary revert.");
			setTime(getTime() + n_network::t_timestamp::epsilon());
                }
		return;
	}
#ifdef SAFETY_CHECKS
	if (this->getTime() > newtime) {
		LOG_ERROR("\tCORE :: ", this->getCoreID() ," Synctime is setting time backward ?? now:", this->getTime(), " new time :", newtime);
		throw std::runtime_error("Core time going backwards. ");
	}
#endif
	
	this->setTime(newtime);															
	this->resetZombieRounds();

	if (this->getTime() >= this->getTerminationTime()) {
		LOG_DEBUG("\tCORE :: ",this->getCoreID() ," Reached termination time :: now: ", this->getTime(), " >= ", this->getTerminationTime());
		this->setLive(false);
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

void n_model::Core::setLive(bool b)
{
        LOG_DEBUG("Core : ", this->getTime(), " id = ", this->getCoreID(), " going to live = ", b);
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
	this->lockSimulatorStep();
        
        m_stats.logStat(TURNS);

	this->getMessages();	// locked on msgs 

	if (!this->isLive()) {
		LOG_DEBUG("\tCORE :: ", this->getCoreID(),
		        " skipping small Step, we're idle and got no messages.");
		this->unlockSimulatorStep();
		return;
	}

        this->getImminent(m_imminents);
        
        // Dynamic structured needs this list, but best before we add externals to it.
        this->signalImminent(m_imminents);  
        
	this->collectOutput(m_imminents);	

	this->getPendingMail();

	this->transition();		

	this->rescheduleImminent();
	
	this->syncTime();				
        m_imminents.clear();
        m_externs.clear();

	this->checkTerminationFunction();

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
        LOG_DEBUG("Termination function == ", fun.get());
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
				this->setTerminationTime(this->getTime());
				this->m_terminated_functor.store(true);
				return;
			}
		}
	} else {
		LOG_DEBUG("\tCORE :: ", this->getCoreID(), " Termination functor == nullptr, not evaluating.");
	}
}

void n_model::Core::removeModel(std::size_t id)
{
        LOG_INFO("\tCORE :: ", this->getCoreID(), " got request to remove model : ", id);
        
        std::swap(m_indexed_models[id], m_indexed_models.back());
        m_indexed_models.pop_back();
        m_heap.remove(id);
        if(id < m_indexed_models.size())
        	m_indexed_models[id]->getUUID().m_local_id = id;
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
        for(t_msgptr ptr : msgs){
                ptr->releaseMe();
                LOG_DEBUG("CORE:: ", this->getCoreID(), " deleting ", ptr);
                m_stats.logStat(DELMSG);
        }
        msgs.clear();
}


void n_model::Core::clearModels()
{
	assert(this->isLive() == false && "Clearing models during simulation is not supported.");
	LOG_DEBUG("\tCORE :: ", this->getCoreID(), " removing all models from core.");
        m_indexed_local_mail.clear();
        m_indexed_models.clear();
	m_heap.clear();

	this->m_received_messages->clear();
	this->setTime(t_timestamp(0, 0));
	this->m_gvt = t_timestamp(0, 0);
}

void n_model::Core::queueLocalMessage(const t_msgptr& msg)
{
        // Don't set flags on a message here, we do this in clearProcessed. (and not at all if we're in single/cons)
        const size_t id = msg->getDestinationModel();
        t_raw_atomic model = this->getModel(id).get();
        LOG_DEBUG("\tCORE :: ", this->getCoreID(), " queueing message to model ", model->getName(), " with id ", id, " it already has messages: ", hasMail(id));
        if(!hasMail(id)){               // If recd msg size==0
        	LOG_DEBUG("\tCORE :: ", this->getCoreID(), " setting it's next transition type to |= external.");
                if(model->nextType()==AtomicModel_impl::NONE){   // If INT is set, we get CONF so adding it to imminents risks duplicate transitions
                        m_externs.push_back(model);     // avoid map by checking state.
                }
                model->markExternal();
        }
        getMail(id).push_back(msg);
}


void n_model::Core::rescheduleAllRevert(const t_timestamp& totime)
{
	for (const auto& model : m_indexed_models) {
		model->revert(totime);
		model->clearSentMessages();
	}
	rescheduleAll();
}

void n_model::Core::rescheduleAll()
{
	LOG_DEBUG("CORE :: ", getCoreID(), " rescheduling all.");
	m_heap.updateAll();
	printSchedulerState();
}


void n_model::Core::receiveMessage(t_msgptr )
{
        assert(false);
}

void n_model::Core::getPendingMail()
{
        if(m_received_messages->empty())
                return;
	const t_timestamp nowtime = makeLatest(m_time);
	std::vector<MessageEntry> messages;
	m_token.getMessage()->setTimeStamp(nowtime);

	this->m_received_messages->unschedule_until(messages, m_token);
	
	for (const auto& entry : messages){
	        auto msg = entry.getMessage();
	        if(msg->flagIsSet(Status::ERASE)){
	                msg->setFlag(Status::KILL);
                        continue;
	        }
                msg->setFlag(Status::HEAPED, false);
                queueLocalMessage(msg);
        }

}


void n_model::Core::setGVT(const t_timestamp& newgvt)
{
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
	        this->isLive());
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
