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
	: m_network(netlink), m_time(0, 0), m_gvt(0,0), m_coreid(id), m_live(false), m_loctable(loc)
{
	m_scheduler = n_tools::SchedulerFactory<ModelEntry>::makeScheduler(n_tools::Storage::BINOMIAL, false);
}

bool n_model::Core::isMessageLocal(const t_msgptr& msg){
	std::string destname = msg->getDestinationModel();
	std::size_t destid = this->m_coreid;
	if(this->m_models.find(destname) != this->m_models.end()){
		msg->setDestinationCore(destid);
	}else{
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

void n_model::Core::init(){
	for(const auto& model : this->m_models){
		t_timestamp model_scheduled_time = model.second->timeAdvance();
		this->scheduleModel(model.first,model_scheduled_time);
	}
	if( not this->m_scheduler->empty()){
		//std::cout << "Core advancing time to first transition time ";
		this->m_time = this->m_scheduler->top().getTime();
		//std::cout << "@" << this->m_time << std::endl;
	}
}

void n_model::Core::collectOutput()
{
	throw std::logic_error("Core : collectOutput not implemented");
	/**
	for(const auto& modelpair : m_models){
		// call outputfunction. // TODO for all or for imminent only ?
	}
	*/
}

void n_model::Core::routeMessages()
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

void n_model::Core::transition()
{
	throw std::logic_error("Core : transition not implemented");
	/**
	 * for(const auto& model : m_models){
	 * 	// Get imminent
	 * 	-> int + ext
	 * 	// Reschedule
	 * 	// TODO is model responsability to see what type transition is required.
	 * 	others : -> ext
	 *
	 * }
	 */
	std::vector<std::string> transitioned;
	this->traceModels(transitioned);
}

void n_model::Core::traceModels(const std::vector<std::string>& transitioned){
	for(const auto& modelname : transitioned){
		auto model = this->m_models[modelname];
		// Trace model;
	}
	// TODO Stijn, link with tracers here.
}

void n_model::Core::printSchedulerState(){
	this->m_scheduler->printScheduler();
}
