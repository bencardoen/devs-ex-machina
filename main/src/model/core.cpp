/*
 * core.cpp
 *
 *      Author: Ben Cardoen
 */
#include <core.h>


void
n_model::Core::load(const std::string&){
	throw std::logic_error("Core : load not implemented");
}

n_model::Core::Core():m_network(nullptr), m_time(0,0){
	;
}

void
n_model::Core::save(const std::string&){
	throw std::logic_error("Core : save not implemented");
}

void
n_model::Core::revert(t_timestamp){
	throw std::logic_error("Core : revert not implemented");
}

void
n_model::Core::addModel(t_modelptr model){
	std::string mname = model->getName();
	assert(this->m_models.find(mname) == this->m_models.end() && "Model allready in core.");
	this->m_models[mname] = model;
}

n_model::t_modelptr
n_model::Core::getModel(std::string mname){
	assert(this->m_models.find(mname) != this->m_models.end() && "Model not in core.");
	return this->m_models[mname];
}

void
n_model::Core::removeModel(std::string mname){
	assert(this->m_models.find(mname) != this->m_models.end() && "Can't remove model in core, not present.");
	this->m_models.erase(mname);
}

void
n_model::Core::receiveMessages(){
	throw std::logic_error("Core : message not implemented");
}

void
n_model::Core::sendMessages(){
	throw std::logic_error("Core : message not implemented");
}
