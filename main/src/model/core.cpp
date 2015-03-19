/*
 * core.cpp
 *
 *      Author: Ben Cardoen
 */
#include <core.h>

void n_model::Core::load(const std::string&)
{
	throw std::logic_error("Core : load not implemented");
}

n_model::Core::Core()
	: m_network(nullptr), m_time(0, 0), m_coreid(0), m_live(false)
{
	m_scheduler = n_tools::SchedulerFactory<ModelEntry>::makeScheduler(n_tools::Storage::BINOMIAL, false);
}

n_model::Core::Core(std::size_t id, const t_networkptr& netlink)
	: m_network(netlink), m_time(0, 0), m_coreid(id), m_live(false)
{
	m_scheduler = n_tools::SchedulerFactory<ModelEntry>::makeScheduler(n_tools::Storage::BINOMIAL, false);
}

void n_model::Core::save(const std::string&)
{
	throw std::logic_error("Core : save not implemented");
}

void n_model::Core::revert(t_timestamp)
{
	throw std::logic_error("Core : revert not implemented");
}

void n_model::Core::addModel(t_modelptr model)
{
	std::string mname = model->getName();
	assert(this->m_models.find(mname) == this->m_models.end() && "Model allready in core.");
	this->m_models[mname] = model;
}

n_model::t_modelptr n_model::Core::getModel(std::string mname)
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

void n_model::Core::collectOutput()
{
	throw std::logic_error("Core : collectOutput not implemented");
	/**
	 for(const auto& modelpair : m_models){
	 // tracing->trace(modelpair.second);
	 }*/
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
}
