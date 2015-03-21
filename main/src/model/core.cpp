/*
 * core.cpp
 *
 *      Author: Ben Cardoen
 */
#include <core.h>
#include <cassert>

void n_model::Core::load(const std::string&)
{
	throw std::logic_error("Core : load not implemented");
}

n_model::Core::Core()
	: m_network(nullptr), m_time(0, 0), m_gvt(0, 0), m_coreid(0), m_live(false), m_loctable(nullptr), m_gehenna(t_timestamp::infinity()),m_terminated(false)
{
	m_scheduler = n_tools::SchedulerFactory<ModelEntry>::makeScheduler(n_tools::Storage::BINOMIAL, false);
}

n_model::Core::Core(std::size_t id, const t_networkptr& netlink, const t_loctableptr& loc)
{
	m_scheduler = n_tools::SchedulerFactory<ModelEntry>::makeScheduler(n_tools::Storage::BINOMIAL, false);
	m_network = netlink;
	m_loctable = loc;
	m_coreid = id;
}

bool n_model::Core::isMessageLocal(const t_msgptr& msg)
{
	std::string destname = msg->getDestinationModel();
	std::size_t destid = this->m_coreid;
	if (this->m_models.find(destname) != this->m_models.end()) {
		msg->setDestinationCore(destid);
	} else {
		// TODO LOOKUP in locationtable and correct coreid
		assert(false && "Lookup of remote model not implemented.");
	}
	return (destid == this->m_coreid);
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

n_model::t_atomicmodelptr n_model::Core::getModel(std::string mname)
{
	assert(this->m_models.find(mname) != this->m_models.end() && "Model not in core.");
	return this->m_models[mname];
}

void n_model::Core::receiveMessages(std::vector<t_msgptr>&)
{
	throw std::logic_error("Core : message not implemented");
}

void n_model::Core::sendMessages()
{
	throw std::logic_error("Core : message not implemented");
}

void n_model::Core::scheduleModel(std::string name, t_timestamp t)
{
	if (this->m_models.find(name) != this->m_models.end()) {
		ModelEntry entry(name, t);
		this->m_scheduler->push_back(entry);
	} else {
		std::cerr << "Model with name " + name + " not in core." << std::endl;
	}
}

void n_model::Core::init()
{
	for (const auto& model : this->m_models) {
		t_timestamp model_scheduled_time = model.second->timeAdvance();
		this->scheduleModel(model.first, model_scheduled_time);
	}
	if (not this->m_scheduler->empty()) {
		//std::cout << "Core advancing time to first transition time ";
		this->m_time = this->m_scheduler->top().getTime();
		//std::cout << "@" << this->m_time << std::endl;
	}
}

void n_model::Core::collectOutput(std::unordered_map<std::string, std::vector<t_msgptr>>& mailbag)
{
	/**
	 * For each model, collect output.
	 * Then sort that output by destination (for the transition functions)
	 */
	std::cout << "Collecting output from all models";
	for(const auto& modelentry : m_models ){
		auto model = modelentry.second;
		auto mailfrom = model->output();
 	 	this->sortMail(mailbag, mailfrom);
	}
	std::cout << " resulted in " << mailbag.size() << " addressees" << std::endl;
}

void
n_model::Core::transition(const std::set<std::string>& imminents, std::unordered_map<std::string, std::vector<t_msgptr>>& mail)
{
	/**
	 * For imminents : if message, confluent, else internal
	 * For others : external
	 */
	std::cout << "Transitioning with " << imminents.size() << " imminents, and " << mail.size() << " models to deliver mail to." << std::endl;
	for(const auto& imminent : imminents){
		t_atomicmodelptr urgent = this->m_models[imminent];
		auto found = mail.find(imminent);
		if(found == mail.end()){
			urgent->intTransition();
			this->traceInt(urgent);
		}else{
			urgent->confTransition(found->second);
			std::size_t erased = mail.erase(imminent);	// Erase so we don't need to double check in the next for loop.
			this->traceConf(urgent);
			assert(erased != 0 && "Broken logic in collected output");
		}
	}

	for(const auto& remaining : mail){
		const t_atomicmodelptr& model = this->m_models[remaining.first];
		model->extTransition(remaining.second);
		this->traceExt(model);
	}
}

void
n_model::Core::sortMail(std::unordered_map<std::string, std::vector<t_msgptr>>& mailbag, const std::vector<t_msgptr>& messages){
	for(const auto & message : messages){
		const std::string& destname = message->getDestinationModel();
		auto found = mailbag.find(destname);
		if(found == mailbag.end()){
			mailbag[destname] = {message};
		}else{
			mailbag[destname].push_back(message);
		}
	}
}

void n_model::Core::printSchedulerState()
{
	this->m_scheduler->printScheduler();
}

std::set<std::string> n_model::Core::getImminent()
{
	std::cout << "Retrieving imminent models" << std::endl;
	std::set<std::string> imminent;
	std::vector<ModelEntry> bag;
	t_timestamp maxtime = n_network::makeLatest(this->m_time);
	ModelEntry mark("", maxtime);
	this->m_scheduler->unschedule_until(bag, mark);
	if (bag.size() == 0)
		std::cerr << "No imminent models ??" << std::endl;
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
	std::cout << "Rescheduling " << oldimms.size() << " models for next run." << std::endl;
	for (const auto& old : oldimms) {
		t_atomicmodelptr model = this->m_models[old];
		t_timestamp ta = model->timeAdvance();
		if(ta != t_timestamp::infinity()){
			t_timestamp next = ta + this->m_time;
			this->scheduleModel(old, next);
		}
		else{
			std::cout << "Core:: " << model->getName() << " is no longer scheduled (infinity) " << std::endl;
		}
	}
	this->syncTime();
}


void n_model::Core::syncTime()
{
	assert(not this->m_scheduler->empty() && "Syncing with the void is illadvised.");
	t_timestamp next = this->m_scheduler->top().getTime();
	std::cout << " Core is advancing simtime to :: " << next << std::endl;
	this->m_time = next;
	if(this->m_time >= this->m_gehenna){
		std::cout << "Reached termination time :: now: " << m_time << " >= "<<  m_gehenna << std::endl;
		m_terminated.store(true);
		m_live.store(false);
	}
}

n_network::t_timestamp n_model::Core::getTime()
{
	return this->m_time;
}

n_network::t_timestamp n_model::Core::getGVT()
{
	return this->m_gvt;
}

bool n_model::Core::isLive() const
{
	return m_live;
}

void n_model::Core::setLive(bool b)
{
	return m_live.store(b);
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

	// Transition depending on state.
	this->transition(imminent, mailbag);

	// Finally find out what next firing times are and place models accordingly.
	// This also sets the kernel time and checks if termination time is reached.
	this->rescheduleImminent(imminent);

	// Do we need to continue ?
	this->checkTerminationFunction();
}

void
n_model::Core::traceInt(const t_atomicmodelptr&){
	// TODO Stijn link with tracer here.
}

void
n_model::Core::traceExt(const t_atomicmodelptr&){
	// TODO Stijn link with tracer here.
}

void
n_model::Core::traceConf(const t_atomicmodelptr&){
	// TODO Stijn link with tracer here.
}

void
n_model::Core::setTerminationTime(t_timestamp endtime){
	assert(endtime > this->m_time && "Error : termination time in the past ??");
	this->m_gehenna = endtime;
}

n_network::t_timestamp
n_model::Core::getTerminationTime()const{return m_gehenna;}

bool
n_model::Core::terminated()const {return m_terminated;}


void
n_model::Core::setTerminationFunction(std::function<bool(const t_atomicmodelptr& model)> fun){
	this->m_termination_function = fun;
}


void
n_model::Core::checkTerminationFunction(){
	if(m_termination_function){
		std::cout << "Checking termination function." << std::endl;
		for(const auto& model : m_models){
			if( m_termination_function(model.second) ){
				std::cout << "Termination function evaluated to true for model " << model.first << std::endl;
				this->m_live.store(false);
				this->m_terminated.store(true);
				return;
			}
		}
	}else{
		std::cout << "No termination function to evaluate, moving on..." << std::endl;
	}
}
