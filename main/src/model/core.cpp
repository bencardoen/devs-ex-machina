/*
 * core.cpp
 *
 *      Author: Ben Cardoen
 */
#include "core.h"
#include <cassert>
#include "globallog.h"
#include "objectfactory.h"

void n_model::Core::load(const std::string&)
{
	throw std::logic_error("Core : load not implemented");
}

n_model::Core::Core()
	: m_time(0, 0), m_gvt(0, 0), m_coreid(0), m_live(false), m_termtime(t_timestamp::infinity()), m_terminated(false)
{
	m_scheduler = n_tools::SchedulerFactory<ModelEntry>::makeScheduler(n_tools::Storage::BINOMIAL, false);
	m_termination_function = n_tools::createObject<n_model::TerminationFunctor>();
}

n_model::Core::Core(std::size_t id)
: m_time(0, 0), m_gvt(0, 0), m_coreid(0), m_live(false), m_termtime(t_timestamp::infinity()), m_terminated(false)
{
	m_scheduler = n_tools::SchedulerFactory<ModelEntry>::makeScheduler(n_tools::Storage::BINOMIAL, false);
	m_coreid = id;
}

bool n_model::Core::isMessageLocal(const t_msgptr& msg)const
{
	const bool found = this->containsModel(msg->getDestinationModel());
	if (found) {
		msg->setDestinationCore(this->getCoreID());
		return true;
	} else {
		assert(false && "Message to model destination not in single core ??");
		return false;
	}
}

void n_model::Core::save(const std::string&)
{
	throw std::logic_error("Core : save not implemented");
}

void n_model::Core::revert(t_timestamp)
{
	throw std::logic_error("Core : revert not implemented");
}

void n_model::Core::addModel(t_atomicmodelptr model)
{
	std::string mname = model->getName();
	assert(this->m_models.find(mname) == this->m_models.end() && "Model allready in core.");
	this->m_models[mname] = model;
}

n_model::t_atomicmodelptr n_model::Core::getModel(const std::string& mname)
{
	assert(this->containsModel(mname) && "Model not in core.");
	return this->m_models[mname];
}

bool n_model::Core::containsModel(const std::string& mname)const
{
	return (this->m_models.find(mname)!=this->m_models.end());
}

void n_model::Core::scheduleModel(std::string name, t_timestamp t)
{
	// Add call to priority for coupled models.
	if (this->m_models.find(name) != this->m_models.end()) {
		ModelEntry entry(name, t);
		this->m_scheduler->push_back(entry);
	} else {
		std::cerr << "Model with name " + name + " not in core, can't reschedule." << std::endl;
	}
}

void n_model::Core::init()
{
	for (const auto& model : this->m_models) {
		t_timestamp model_scheduled_time = model.second->timeAdvance();
		this->scheduleModel(model.first, model_scheduled_time);
	}
	if (not this->m_scheduler->empty()) {
		this->m_time = this->m_scheduler->top().getTime();
		LOG_INFO("Core initialized to first time : ", this->m_time);
	}
}

void n_model::Core::collectOutput(std::unordered_map<std::string, std::vector<t_msgptr>>& mailbag)
{
	/**
	 * For each model, collect output.
	 * Then sort that output by destination (for the transition functions)
	 */
	LOG_DEBUG("CORE: Collecting output from all models");
	for (const auto& modelentry : m_models) {
		const auto& model = modelentry.second;
		auto mailfrom = model->output();
		this->sortMail(mailbag, mailfrom);
	}
	LOG_DEBUG("CORE:  resulted in ",  mailbag.size(), " addressees");
}

void n_model::Core::transition(std::set<std::string>& imminents,
        std::unordered_map<std::string, std::vector<t_msgptr>>& mail)
{
	/**
	 * For imminents : if message, confluent, else internal
	 * For others : external
	 */
	LOG_DEBUG("CORE: Transitioning with ",  imminents.size(), " imminents, and ", mail.size(), " models to deliver mail to.");
	for (const auto& imminent : imminents) {
		t_atomicmodelptr urgent = this->m_models[imminent];
		const auto& found = mail.find(imminent);
		if (found == mail.end()) {
			urgent->intTransition(); // TODO check with Stijn.
			urgent->setTime(this->m_time);
			this->traceInt(urgent);
		} else {
			urgent->confTransition(found->second);
			urgent->setTime(this->m_time);
			std::size_t erased = mail.erase(imminent); // Erase so we don't need to double check in the next for loop.
			this->traceConf(urgent);
			assert(erased != 0 && "Broken logic in collected output");
		}
	}

	for (const auto& remaining : mail) {
		const t_atomicmodelptr& model = this->m_models[remaining.first];
		model->extTransition(remaining.second);
		model->setTime(this->m_time);
		t_timestamp queried = model->timeAdvance();
		// If a model 'wakes up' after an event (ta goes from infinity to a scheduleable value), make sure we reschedule it.
		if(queried != t_timestamp::infinity()){
			LOG_INFO("Model ", model->getName(), " changed ta value from infinity to ", queried, " rescheduling.");
			imminents.insert(model->getName());
		}
		this->traceExt(model);
	}
}

void n_model::Core::sortMail(std::unordered_map<std::string, std::vector<t_msgptr>>& mailbag,
        const std::vector<t_msgptr>& messages)
{
	for (const auto & message : messages) {
		if(not this->isMessageLocal(message)){
			this->sendMessage(message);
		}
		const std::string& destname = message->getDestinationModel();
		auto found = mailbag.find(destname);
		if (found == mailbag.end()) {
			mailbag[destname] = {message};
		} else {
			mailbag[destname].push_back(message);
		}
	}
	// At this point we have send all remote messages, sorted the rest for local delivery.
	// Add a hook to multicore pull from network.
	getMessages(mailbag);
}

void n_model::Core::printSchedulerState()
{
	this->m_scheduler->printScheduler();
}

std::set<std::string> n_model::Core::getImminent()
{
	LOG_DEBUG("CORE: Retrieving imminent models");
	std::set<std::string> imminent;
	std::vector<ModelEntry> bag;
	t_timestamp maxtime = n_network::makeLatest(this->m_time);
	ModelEntry mark("", maxtime);
	this->m_scheduler->unschedule_until(bag, mark);
	if (bag.size() == 0){
		LOG_ERROR("CORE: No imminent models ??");
	}
	for (const auto& entry : bag) {
		bool inserted = imminent.insert(entry.getName()).second;
		assert(inserted && "Logic fail in Core get Imminent.");
	}
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
		t_timestamp ta = model->timeAdvance();			// Timeadvance if infinity == model indicates it does not want scheduling.
		if (ta != t_timestamp::infinity()) {
			t_timestamp next = ta + this->m_time;
			size_t prior = model->getPriority();		// Simulate select function for models firing simultaneously by changing causality vals.
			next.increaseCausality(prior);
			this->scheduleModel(old, next);
		} else {
			LOG_DEBUG("CORE: Core:: ", model->getName(), " is no longer scheduled (infinity) ");
		}
	}
	this->syncTime();
}

void n_model::Core::syncTime()
{
	if(not this->m_scheduler->empty()){
		t_timestamp next = this->m_scheduler->top().getTime();
		this->m_time = this->m_time + next;
		LOG_DEBUG("CORE:  Core is advancing simtime to :: ", this->m_time.getTime());
	}else{
		LOG_WARNING("CORE:: Core has no scheduled models, time is no longer advancing.");
	}

	this->adjustTime(); // Allow subclasses to do their own thing here.

	if (this->m_time >= this->m_termtime) {
		LOG_DEBUG("CORE: Reached termination time :: now: ", m_time, " >= ", m_termtime);
		m_terminated.store(true);
		m_live.store(false);
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
	assert(this->m_live && "Attempted to run a simulation step in a dead kernel ?");

	// Query imminent models (who are about to fire transition)
	auto imminent = this->getImminent();

	// Get all produced messages, and route them.
	std::unordered_map<std::string, std::vector<t_msgptr>> mailbag;
	this->collectOutput(mailbag);

	// Noop in single core. Pull messages from network, sort them.
	this->getMessages(mailbag);

	// Transition depending on state.
	this->transition(imminent, mailbag);

	// Finally find out what next firing times are and place models accordingly.
	// This also sets the kernel time and checks if termination time is reached.
	this->rescheduleImminent(imminent);

	// Do we need to continue ?
	this->checkTerminationFunction();
}

void n_model::Core::traceInt(const t_atomicmodelptr& /* model */)
{
	// TODO Stijn uncomment
	// m_tracerset->tracesInternal(model);

}

void n_model::Core::traceExt(const t_atomicmodelptr& /* model */)
{
	// TODO Stijn uncomment
	// m_tracerset->tracesExternal(model);
}

void n_model::Core::traceConf(const t_atomicmodelptr& /* model */)
{
	// TODO Stijn uncomment
	// m_tracerset->tracesConfluent(model);
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
				this->m_live.store(false);
				this->m_terminated.store(true);
				return;
			}
		}
	} else {
		LOG_ERROR("CORE: Termination functor == nullptr, not evaluating.");
	}
}

void n_model::Core::removeModel(std::string name){
	assert(this->isLive()==false && "Can't remove model from live core.");
	std::size_t erased = this->m_models.erase(name);
	assert(erased!=0 && "Trying to remove model not in this core ??");
	ModelEntry target(name, t_timestamp(0,0));
	this->m_scheduler->erase(target);
	assert(this->m_scheduler->contains(target)==false && "Removal from scheduler failed !!");
}

void
n_model::Core::setTime(const t_timestamp& t){
	m_time = t;
}
