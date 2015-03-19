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
	: m_network(nullptr), m_time(0, 0), m_gvt(0, 0), m_coreid(0), m_live(false), m_loctable(nullptr)
{

	m_scheduler = n_tools::SchedulerFactory<ModelEntry>::makeScheduler(n_tools::Storage::BINOMIAL, false);
}

n_model::Core::Core(std::size_t id, const t_networkptr& netlink, const t_loctableptr& loc)
	: m_network(netlink), m_time(0, 0), m_gvt(0, 0), m_coreid(id), m_live(false), m_loctable(loc)
{
	m_scheduler = n_tools::SchedulerFactory<ModelEntry>::makeScheduler(n_tools::Storage::BINOMIAL, false);
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

void n_model::Core::collectOutput()
{
	throw std::logic_error("Core : collectOutput not implemented");
	/**
	 for each model:
	 messages = model.outputfunction();
	 routeMessages(messages);
	 */
}

void n_model::Core::routeMessages(const std::map<t_portptr, n_network::t_msgptr>&)
{
	throw std::logic_error("Core : routeMessages not implemented");
	/**
	 // Collect all messages (output function)
	 for(const auto& modelpair : m_models){
	 //auto messagemap = modelpair.second->output();
	 }
	 // Send messages
	 for collected messages : lookup destination
	 if destination local :
	 route direct
	 else
	 sendMessage()
	 // Receive pending messages
	 */
	//m_network->getMessages(this->m_coreid);
}

std::set<std::string> n_model::Core::transition(const std::set<std::string>&)
{
	std::set<std::string> transitioned;
	throw std::logic_error("Core : transition not implemented");
	/**
	 * std::vector<std::string> transitioned;
	 * for model in imminent:
	 * 	if(got external)
	 * 		model.confluent();
	 * 	else
	 * 		model.internal()
	 * 	transitioned.insert(model.getName());
	 * for model in models && not in imminent:
	 * 	if(got external
	 * 		model.external();
	 * 	transitioned.insert(model.getname());
	 */
	return transitioned;
}

void n_model::Core::traceModels(const std::set<std::string>& transitioned)
{
	for (const auto& modelentry : transitioned) {
		auto model = this->m_models[modelentry];
		// Trace model;
	}
	// TODO Stijn, link with tracers here.
}

void n_model::Core::printSchedulerState()
{
	this->m_scheduler->printScheduler();
}

std::set<std::string> n_model::Core::getImminent()
{
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
	for (const auto& old : oldimms) {
		t_atomicmodelptr model = this->m_models[old];
		t_timestamp next = model->timeAdvance() + this->m_time;
		this->scheduleModel(old, next);
	}
	this->syncTime();
}

/**
 * Updates local time from first entry in scheduler.
 * @attention : if scheduler is empty this will crash. (it should)
 */
void n_model::Core::syncTime()
{
	assert(not this->m_scheduler->empty() && "Syncing with the void is illadvised.");
	t_timestamp next = this->m_scheduler->top().getTime();
	//std::cout << " Core is advancing simtime to :: " << next << std::endl;
	this->m_time = next;
	// TODO check gvt etc..
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
	this->collectOutput();
	// Transition if/when necessary
	auto transitioned = this->transition(imminent);
	// Trace what happened in this step.
	this->traceModels(transitioned);
	// Finally find out what next firing times are and place models accordingly.
	// This also sets the kernel time.
	this->rescheduleImminent(imminent);
}

void n_model::Core::checkCoreIntegrity(){
	for(const auto& modelname : m_models){
		assert(this->m_scheduler->contains(ModelEntry(modelname.first, t_timestamp(0,0))));
	}
}
