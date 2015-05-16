/*
 * modela.cpp
 *
 *  Created on: May 16, 2015
 *      Author: tim
 */

#include <modela.h>

namespace n_examples_abstract_c {

ModelA::ModelA(std::string name, std::size_t priority)
	: AtomicModel(name, priority)
{
	this->setState(n_tools::createObject<State>("0"));
	m_elapsed = 0;
	this->addOutPort("A");
}

ModelA::~ModelA()
{
}

void ModelA::extTransition(const std::vector<n_network::t_msgptr> & message)
{
	// No external transitions available
}

void ModelA::intTransition()
{
	t_stateptr state = this->getState();
	if (*state == "0")
		this->setState("1");
	else if (*state == "1")
		this->setState("2");
	else if (*state == "2")
		this->setState("3");
	else if (*state == "3")
		this->setState("4");
	else if (*state == "4")
		this->setState("5");
	else if (*state == "5")
		this->setState("6");
	else if (*state == "6")
		this->setState("7");
	else
		assert(false); // You shouldn't come here...
	return;
}

t_timestamp ModelA::timeAdvance() const
{
	t_stateptr state = this->getState();
	if (*state == "7")
		return t_timestamp::infinity();
	return t_timestamp(10);
}

std::vector<n_network::t_msgptr> ModelA::output() const
{
	t_stateptr state = this->getState();
	if ((*state == "2") || (*state == "5")) {
		return this->getPort("A")->createMessages("start");
	}
	return std::vector<n_network::t_msgptr>();
}

t_timestamp ModelA::lookAhead() const
{
	return t_timestamp::infinity();
}

t_stateptr ModelA::setState(std::string s)
{
	this->Model::setState(n_tools::createObject<State>(s));
	return this->getState();
}

} /* namespace n_examples_abstract_c */
