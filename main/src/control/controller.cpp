/*
 * Controller.cpp
 *
 *  Created on: 11 Mar 2015
 *      Author: matthijs
 */

#include <control/controller.h>

namespace n_control {

Controller::Controller(std::string name)
	: m_name(name), m_isClassicDEVS(true), m_checkTermTime(false), m_checkTermCond(false)
{
}

Controller::~Controller()
{
}

void Controller::addModel(t_modelPtr model)
{
	//TODO
}

void Controller::simulate()
{
	while (true) {
		//TODO
	}

}

void Controller::setClassicDEVS(bool classicDEVS = true)
{
	m_isClassicDEVS = classicDEVS;
}

void Controller::setTerminationTime(n_network::Time time)
{
	m_checkTermTime = true;
	m_terminationTime = time;
}

void Controller::setTerminationCondition(std::function<bool(n_network::Time, n_model::Model)> termination_condition)
{
	m_checkTermCond = true;
	m_terminationCondition = termination_condition;
}

bool Controller::check()
{
	if(m_checkTermTime) {
		//TODO
	}
	if(m_checkTermCond) {
		//TODO
	}
	return false;
}

} /* namespace n_control */
