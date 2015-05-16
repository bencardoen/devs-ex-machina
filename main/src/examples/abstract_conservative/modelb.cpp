/*
 * modelb.cpp
 *
 *  Created on: May 16, 2015
 *      Author: tim
 */

#include <modelb.h>

namespace n_examples_abstract_c {

ModelB::ModelB(std::string name, std::size_t priority)
	: AtomicModel(name, priority)
{
	this->setState(n_tools::createObject<State>("0"));
	m_elapsed = 0;
	this->addInPort("B");
}

ModelB::~ModelB()
{
}

void ModelB::extTransition(const std::vector<n_network::t_msgptr> & message)
{
	t_stateptr state = this->getState();
	if ((*state == "2") && (message.at(0)->getPayload() == "start"))
		this->setState("3");
	else if ((*state == "5") && (message.at(0)->getPayload() == "start"))
		this->setState("6");
}

void ModelB::intTransition()
{
	t_stateptr state = this->getState();
	if (*state == "0")
		this->setState("1");
	else if (*state == "1")
		this->setState("2");
	else if (*state == "3")
		this->setState("4");
	else if (*state == "4")
		this->setState("5");
	else if (*state == "6")
		this->setState("7");
	else
		assert(false); // You shouldn't come here...
	return;
}

t_timestamp ModelB::timeAdvance() const
{
	t_stateptr state = this->getState();
	if ((*state == "7") || (*state == "2") || (*state == "5"))
		return t_timestamp::infinity();
	return t_timestamp(10);
}

std::vector<n_network::t_msgptr> ModelB::output() const
{
	return std::vector<n_network::t_msgptr>();
}

t_timestamp ModelB::lookAhead() const
{
	t_stateptr state = this->getState();
	if ((*state == "0") || (*state == "3")){
		return t_timestamp(30);
	}
	// TODO : BEN, ik denk niet dat je ooit de lookahead van deze volgende staten
	// 	  	(1,4,2,5) zult moeten oproepen aangezien je met de lookahead van 30
	//		al genoeg weet, check dit wel!
	// DONE : Tim, als je assert(false) in lookahead 1,4,2,5 zet faalt de simulatie vrijwel direct.
	// De lookahead wordt altijd opgeroepen, ook als je een externe transitie doet (je staat verandert immers)
	// en dus ook (mogelijk) je lookahead.
	else if ((*state == "1") || (*state == "4")){
		return t_timestamp(20);
	}
	else if ((*state == "2") || (*state == "5")){
		return t_timestamp(10);
	}

	return t_timestamp::infinity();
}

t_stateptr ModelB::setState(std::string s)
{
	this->Model::setState(n_tools::createObject<State>(s));
	return this->getState();
}

} /* namespace n_examples_abstract_c */


