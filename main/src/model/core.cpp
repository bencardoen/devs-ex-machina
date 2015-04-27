/*
 * core.cpp
 *
 *      Author: Ben Cardoen
 */
#include "core.h"
#include <cassert>
#include "globallog.h"
#include "objectfactory.h"

using n_network::MessageEntry;

n_model::Core::~Core()
{
	// Make sure we don't keep stale pointers alive
	for (auto& model : m_models) {
		model.second.reset();
	}
	m_models.clear();
	m_received_messages->clear();
}

void n_model::Core::load(const std::string&)
{
	throw std::logic_error("Core : load not implemented");
}

n_model::Core::Core()
	: m_time(0, 0), m_gvt(0, 0), m_coreid(0), m_live(false), m_termtime(t_timestamp::infinity()), m_terminated(
	        false), m_idle(false)
{
	m_received_messages = n_tools::SchedulerFactory<MessageEntry>::makeScheduler(n_tools::Storage::BINOMIAL, true);
	m_scheduler = n_tools::SchedulerFactory<ModelEntry>::makeScheduler(n_tools::Storage::BINOMIAL, true);
	m_termination_function = n_tools::createObject<n_model::TerminationFunctor>();
}

n_model::Core::Core(std::size_t id)
	: Core()
{
	m_coreid = id;
	assert(m_time == t_timestamp(0, 0));
	assert(m_live == false);
}

bool n_model::Core::isMessageLocal(const t_msgptr& msg) const
{
	/**
	 *  Got a generated message from my own models, figure out if destination is local.
	 */
	const bool found = this->containsModel(msg->getDestinationModel());
	if (found) {
		msg->setDestinationCore(this->getCoreID());
		return true;
	} else {
		return false;
	}
}

void n_model::Core::save(const std::string&)
{
	throw std::logic_error("Core : save not implemented");
}

void n_model::Core::addModel(t_atomicmodelptr model)
{
	std::string mname = model->getName();
	assert(this->m_models.find(mname) == this->m_models.end() && "Model already in core.");
	this->m_models[mname] = model;
}

n_model::t_atomicmodelptr n_model::Core::getModel(const std::string& mname)
{
	assert(this->containsModel(mname) && "Model not in core.");
	return this->m_models[mname];
}

bool n_model::Core::containsModel(const std::string& mname) const
{
	return (this->m_models.find(mname) != this->m_models.end());
}

void n_model::Core::scheduleModel(std::string name, t_timestamp t)
{
	if (this->m_models.find(name) != this->m_models.end()) {
		LOG_DEBUG("CORE :: ", this->getCoreID(), " got request rescheduling : ", name, "@", t);
		size_t offset = this->m_models[name]->getPriority();
		t_timestamp newt(t.getTime(), offset);
		ModelEntry entry(name, newt);
		LOG_DEBUG("CORE :: ", this->getCoreID(), " rescheduling : ", name, "@", newt);
		if (this->m_scheduler->contains(entry)) {
			LOG_INFO("CORE:: ", this->getCoreID(), " scheduleModel Tried to schedule a model that is already scheduled: ", name,
			" at t=", newt, " replacing.");
			this->m_scheduler->erase(entry);			// Needed for revert, scheduled entry may be wrong.
		}
		this->m_scheduler->push_back(entry);	// assert fail possible if if clause is hit. Do not move to else!
	} else {
		LOG_ERROR("CORE :: ", this->getCoreID(), " !!LOGIC ERROR!! Model with name "
			, name, " not in core, can't reschedule.");
	}
}

void n_model::Core::init()
{
	if (this->m_scheduler->size() != 0) {
		LOG_ERROR(
		        "CORE:: scheduler is not empty on call to init(), are you calling it multiple times ? Hint: don't!!");
		return;
	}
	for (const auto& model : this->m_models) {
		LOG_DEBUG("Core :", this->getCoreID(), " has ", model.first);
	}
	for (const auto& model : this->m_models) {
		t_timestamp modelTime(this->getTime().getTime() - model.second->getTimeElapsed().getTime(),0);
		model.second->setTime(modelTime);	// DO NOT use priority, model does this already
		t_timestamp model_scheduled_time = model.second->getTimeNext(); // model.second->timeAdvance();
		this->scheduleModel(model.first, model_scheduled_time);
		m_tracers->tracesInit(model.second, t_timestamp(0, model.second->getPriority()));
	}
	// This avoid problems with reverting to before first core time, which breaks the models.
	// [60,110]
	this->m_gvt = (this->getTime().getTime(), 0);
}

void n_model::Core::collectOutput(std::set<std::string>& imminents)
{
	/**
	 * For each imminent model, collect output.
	 * Then sort that output by destination (for the transition functions)
	 */
	LOG_DEBUG("CORE: ", this->getCoreID(), " Collecting output for ", imminents.size(), " imminents ");
	for (const auto& modelname : imminents) {
		const auto& model = m_models[modelname];
		auto mailfrom = model->doOutput();
		LOG_DEBUG("CORE:", this->getCoreID(), " got ", mailfrom.size(), " messages from ", modelname);
		// Set timetstamp, source and color (info model does not have).
		for (const auto& msg : mailfrom) {
			msg->setSourceCore(this->getCoreID());
			paintMessage(msg);
			msg->setTimeStamp(this->getTime());
		}
		this->sortMail(mailfrom);	// <-- Locked here on msglock
	}
}

void n_model::Core::transition(std::set<std::string>& imminents,
        std::unordered_map<std::string, std::vector<t_msgptr>>& mail)
{
	// Imminents : need at least internal transition
	// Mail : models with pending messages (ext or confluent)
	LOG_DEBUG("CORE: ", this->getCoreID(), " Transitioning with ", imminents.size(), " imminents, and ",
	        mail.size(), " models to deliver mail to.");
	t_timestamp noncausaltime(this->getTime().getTime(), 0);
	for (const auto& imminent : imminents) {
		t_atomicmodelptr urgent = this->m_models[imminent];
		const auto& found = mail.find(imminent);
		if (found == mail.end()) {				// Internal
			urgent->intTransition();
			urgent->setTime(noncausaltime);
			this->traceInt(urgent);
		} else {
			urgent->confTransition(found->second);		// Confluent
			urgent->setTime(noncausaltime);
			this->traceConf(urgent);
			this->markProcessed(found->second);		// Store message as processed for timewarp.
			std::size_t erased = mail.erase(imminent); // Erase so we don't need to double check in the next for loop.
			assert(erased != 0 && "Broken logic in collected output");
		}
	}

	for (const auto& remaining : mail) {				// External
		const t_atomicmodelptr& model = this->m_models[remaining.first];
		model->doExtTransition(remaining.second);
		model->setTime(noncausaltime);
		// Erase scheduled entry. If external changes ta -> oo, we won't need it anymore.
		// Else if ta->oo, the scheduled entry will be incorrect, reschedule will not fix this (not called).
		// Else if ta = same/something else, we'll reschedule it. O(1).
		m_scheduler->erase(ModelEntry(model->getName(), this->getTime()));
		this->traceExt(model);
		this->markProcessed(remaining.second);
		t_timestamp queried = model->timeAdvance();// A previously inactive model can be awoken, make sure we check this.
		if (queried != t_timestamp::infinity()) {
			LOG_INFO("CORE: ", this->getCoreID(), " Model ", model->getName(),
				" changed ta value to ", queried, " rescheduling.");
			imminents.insert(model->getName());
		}else{
			LOG_INFO("CORE: ", this->getCoreID(), " Model ", model->getName(),
				" changed ta value to infinity, no longer scheduling.");
		}
	}
}

void n_model::Core::sortMail(const std::vector<t_msgptr>& messages)
{
	this->lockMessages();
	for (const auto & message : messages) {
		LOG_DEBUG("CORE: ", this->getCoreID(), " sorting message ", message->toString());
		if (not this->isMessageLocal(message)) {
			this->sendMessage(message);	// A noop for single core, multi core handles this.
		} else {
			this->receiveMessage(message);
		}
	}
	this->unlockMessages();
}

void n_model::Core::printSchedulerState()
{
	this->m_scheduler->printScheduler();
}

std::set<std::string> n_model::Core::getImminent()
{
	LOG_DEBUG("CORE: ", this->getCoreID(), " Retrieving imminent models ");
	std::set<std::string> imminent;
	std::vector<ModelEntry> bag;
	t_timestamp maxtime = n_network::makeLatest(this->m_time);
	ModelEntry mark("", maxtime);
	this->m_scheduler->unschedule_until(bag, mark);
	if (bag.size() == 0) {
		LOG_WARNING("CORE: ", this->getCoreID(), " No imminent models ?? ");
	}
	for (const auto& entry : bag) {
		bool inserted = imminent.insert(entry.getName()).second;
		assert(inserted && "Logic fail in Core get Imminent.");
	}
	LOG_DEBUG("CORE: ", this->getCoreID(), " Have ", imminent.size(), " imminents ");
	return imminent;
}

/**
 * Asks for each unscheduled model a new firing time and places items on the scheduler.
 */
void n_model::Core::rescheduleImminent(const std::set<std::string>& oldimms)
{
	LOG_DEBUG("CORE: Rescheduling ", oldimms.size(), " models for next run.");
	for (const auto& old : oldimms) {
		assert(this->containsModel(old));
		t_atomicmodelptr model = this->m_models[old];
		t_timestamp ta = model->timeAdvance();// Timeadvance if infinity == model indicates it does not want scheduling.
		if (ta != t_timestamp::infinity()) {
			t_timestamp next = ta + this->m_time;
			LOG_DEBUG("CORE: ", this->getCoreID(), " ", model->getName(), " timeadv = ", ta,
			        " rescheduled @ ", next);
			this->scheduleModel(old, next);
		} else {
			LOG_INFO("CORE: Core:: ", model->getName(), " is no longer scheduled (infinity) ");
		}
	}
}

void n_model::Core::syncTime()
{
	t_timestamp nextfired = t_timestamp::infinity();
	if (not this->m_scheduler->empty()) {
		nextfired = this->m_scheduler->top().getTime();
	}
	t_timestamp firstmessagetime = this->getFirstMessageTime();	// Locked on msgs.
	LOG_DEBUG("CORE ", this->getCoreID(), " Candidate for new time is min( ", nextfired, " , ", firstmessagetime);
	t_timestamp newtime = std::min(firstmessagetime, nextfired);
	if (newtime == t_timestamp::infinity()) {
		LOG_WARNING("CORE:: ", this->getCoreID(), "Core has no new time (no msgs, no scheduled models), marking as zombie");
		this->m_zombie_rounds.fetch_add(1);
		return;
	}
	if (this->getTime() > newtime) {
		LOG_ERROR("CORE:: Synctime is setting time backward ?? now:", this->getTime(), " new time :", newtime);
		assert(false);	// crash hard.
	}
	// Here we a valid new time.
	this->setTime(newtime);
	this->m_zombie_rounds.store(0);					// reset zombie state.

	if (this->m_time >= this->m_termtime) {
		LOG_DEBUG("CORE: Reached termination time :: now: ", m_time, " >= ", m_termtime);
		this->setTerminated(true);
		this->setLive(false);
		this->setIdle(true);
	}
}

n_network::t_timestamp n_model::Core::getTime() const
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
	LOG_DEBUG("Core :: ", this->getCoreID(), " setting state to idle=", idlestat);
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

void n_model::Core::runSmallStep()
{
	// Lock simulator to allow setGVT/Revert to clear things up.
	this->lockSimulatorStep();

	// Noop in single core. Pull messages from network, sort them.
	// This step can trigger a revert, which is why its before getImminent
	this->getMessages();	// locked on msgs

	if (this->isIdle()) {// If we're done, but the others aren't, check if we have reverted. If not, skip rest of work.
		LOG_DEBUG("Core:: ", this->getCoreID(),
		        " skipping small Step, we're idle (and messages did not alter that)");
		this->unlockSimulatorStep();
		return;
	}

	// Query imminent models (who are about to fire transition)
	auto imminent = this->getImminent();

	// Get all produced messages, and route them.
	this->collectOutput(imminent);			// locked on msgs

	// Give DynStructured Devs a chance to store imminent models.
	this->signalImminent(imminent);

	// Get msg < timenow, sort them for ext/conf.
	std::unordered_map<std::string, std::vector<t_msgptr>> mailbag;
	this->getPendingMail(mailbag);			// locked on msgs

	// Transition depending on state.
	this->transition(imminent, mailbag);		// NOTE: the scheduler can go empty() here.

	// Finally find out what next firing times are and place models accordingly.
	this->rescheduleImminent(imminent);

	// Forward time to next message/firing.
	this->syncTime();				// locked on msgs

	// Do we need to continue ?
	this->checkTerminationFunction();

	// Finally, unlock simulator.
	this->unlockSimulatorStep();
}

void n_model::Core::traceInt(const t_atomicmodelptr& model)
{
	if (not this->m_tracers) {
		LOG_WARNING("CORE:: ", "i have no tracers ?? , tracerset = nullptr.");
	} else {
		this->m_tracers->tracesInternal(model, this->getCoreID());
	}
}

void n_model::Core::traceExt(const t_atomicmodelptr& model)
{
	if (not this->m_tracers) {
		LOG_WARNING("CORE:: ", "i have no tracers ?? , tracerset = nullptr.");
	} else {
		this->m_tracers->tracesExternal(model, this->getCoreID());
	}
}

void n_model::Core::traceConf(const t_atomicmodelptr& model)
{
	if (not this->m_tracers) {
		LOG_WARNING("CORE:: ", "i have no tracers ?? , tracerset = nullptr.");
	} else {
		this->m_tracers->tracesConfluent(model, this->getCoreID());
	}
}

void n_model::Core::setTerminationTime(t_timestamp endtime)
{
	assert(endtime > this->m_time && "Error : termination time in the past ??");
	this->m_termtime = endtime;
}

n_network::t_timestamp n_model::Core::getTerminationTime() const
{
	return m_termtime;
}

bool n_model::Core::terminated() const
{
	return m_terminated;
}

void n_model::Core::setTerminated(bool b)
{
	m_terminated.store(b);
}

void n_model::Core::setTerminationFunction(const t_terminationfunctor& fun)
{
	this->m_termination_function = fun;
}

void n_model::Core::checkTerminationFunction()
{
	if (m_termination_function) {
		LOG_DEBUG("CORE: Checking termination function.");
		for (const auto& model : m_models) {
			if ((*m_termination_function)(model.second)) {
				LOG_DEBUG("CORE: Termination function evaluated to true for model ", model.first);
				this->setLive(false);
				this->setIdle(true);
				this->setTerminated(true);
				return;
			}
		}
	} else {
		LOG_WARNING("CORE: Termination functor == nullptr, not evaluating.");
	}
}

void n_model::Core::removeModel(std::string name)
{
	if (this->containsModel(name)) {
		std::size_t erased = this->m_models.erase(name);
		assert(erased > 0 && "Failed to erase model ??");
		ModelEntry target(name, t_timestamp(0, 0));
		this->m_scheduler->erase(target);
		LOG_INFO("Core :: ", this->getCoreID(), " removed model : ", name);
		assert(this->m_scheduler->contains(target) == false && "Removal from scheduler failed !!");
	} else {
		LOG_WARNING("Core :: you've asked to remove model with name ", name, " which is not in this core.");
	}
}

void n_model::Core::setTime(const t_timestamp& t)
{
	LOG_DEBUG("CORE:: ", this->getCoreID(), " setting time from ", m_time, " to ", t);
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
	LOG_DEBUG("CORE:: ", this->getCoreID(), " asking tracers to write output up to ", marktime);
	n_tracers::traceUntil(marktime);
}

void n_model::Core::clearModels()
{
	assert(this->isLive() == false && "Clearing models during simulation is not supported.");
	LOG_DEBUG("CORE:: ", this->getCoreID(), " removing all models from core.");
	this->m_models.clear();
	this->m_scheduler->clear();
	this->m_received_messages->clear();
	this->setTime(t_timestamp(0, 0));
	this->m_gvt = t_timestamp(0, 0);
}

void n_model::Core::queuePendingMessage(const t_msgptr& msg)
{
	MessageEntry entry(msg);
	this->m_received_messages->push_back(entry);
}

void n_model::Core::rescheduleAll(const t_timestamp& totime)
{
	this->m_scheduler->clear();
	assert(m_scheduler->empty());
	for (const auto& modelentry : m_models) {
		t_timestamp modellast = modelentry.second->revert(totime);
		// Bug lived here : Do not set time on model.
		if (modellast != t_timestamp::infinity()) {
			this->scheduleModel(modelentry.first, modellast);
		} else {
			LOG_WARNING("MCore:: model did not give a new time for rescheduling after revert",
			        modelentry.second->getName());
		}
	}
}

void n_model::Core::receiveMessage(const t_msgptr& msg)
{
	LOG_DEBUG("CORE:: ", this->getCoreID(), " receiving message", msg->toString());
	t_timestamp msgtime = msg->getTimeStamp();
	if (msg->isAntiMessage()) {
		this->handleAntiMessage(msg);	// wipes message if it exists in pending, timestamp is checked later.
	} else {
		// Don't store antimessages, we're partially ordered, there is no way a sent messages can be hopped over in a FIFO by its antimessage.
		this->queuePendingMessage(msg);
	}

	// Either antimessage < time, or plain message < time, trigger revert AFTER saving msg.
	if (msgtime < this->getTime()) {
		LOG_INFO("Core:: ", this->getCoreID(), " received message time < than now : ", this->getTime(),
		        " msg follows: ", msg->toString());
		this->revert(msg->getTimeStamp());
	}
}

void n_model::Core::getPendingMail(std::unordered_map<std::string, std::vector<t_msgptr>>& mailbag)
{
	/**
	 * Check if we have pending messages with time <= (now, oo);
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
		if (not this->containsModel(modelname)) {			//DynSDevs : filter void messages
			continue;
		} else {
			if (mailbag.find(modelname) == mailbag.end()) {
				mailbag[modelname] = std::vector<t_msgptr>();	// Only make them if we have mail.
			}
			mailbag[modelname].push_back(entry.getMessage());
		}
	}
}

t_timestamp n_model::Core::getFirstMessageTime()
{
	this->lockMessages();
	t_timestamp mintime = t_timestamp::infinity();
	while (not this->m_received_messages->empty()) {// We need to skip messages with destmodel not in this core (dyn structured)
		MessageEntry first = this->m_received_messages->top();
		std::string modeldest = first.getMessage()->getDestinationModel();
		if (this->containsModel(modeldest)) {
			this->unlockMessages();	// second exit path, don't forget to unlock.
			return first.getMessage()->getTimeStamp();
		} else {
			LOG_DEBUG("Core : ", this->getCoreID(), " removing message from msgqueue with destination ",
			        modeldest);
			this->m_received_messages->pop();
		}
	}
	this->unlockMessages();
	// Get here if we have no valid messages.
	return mintime;
}

void n_model::Core::setGVT(const t_timestamp& newgvt)
{
	if (newgvt == t_timestamp::infinity()) {
		LOG_WARNING("CORE:: ", this->getCoreID(), " received request to set gvt to infinity, ignoring.");
		return;
	}
	if (newgvt < this->getGVT()) {
		LOG_WARNING("CORE:: ", this->getCoreID(), " received request to set gvt to ", newgvt, " < ",
		        this->getGVT(), " ignoring ");
		return;
	}
	LOG_DEBUG("Core: ", this->getCoreID(), " Setting gvt from ::", this->getGVT(), " to ", newgvt);
	this->m_gvt = newgvt;
}

void n_model::Core::printPendingMessages()
{
	this->lockMessages();
	this->m_received_messages->printScheduler();
	this->unlockMessages();
}

void n_model::Core::revertTracerUntil(const t_timestamp& totime)
{
	n_tracers::revertTo(totime, this->getCoreID());
}

void n_model::Core::logCoreState()
{
	LOG_DEBUG("Core: ", this->getCoreID(), " time= ", this->getTime(), " gvt=", this->getGVT(), " live=",
	        this->isLive(), " idle=", this->isIdle(), " terminated=", this->terminated());
}


bool
n_model::Core::existTransientMessage(){
	LOG_ERROR("Core: ", this->getCoreID(), " existTransientMessage called on single core.");
	assert(false);
}

std::size_t
n_model::Core::getZombieRounds(){
	return m_zombie_rounds;
}
