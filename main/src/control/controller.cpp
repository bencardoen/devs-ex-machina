/*
 * Controller.cpp
 *
 *  Created on: 11 Mar 2015
 *      Author: matthijs
 */

#include "controller.h"

namespace n_control {

Controller::Controller(std::string name)
	:m_isClassicDEVS(true), m_name(name), m_checkTermTime(false), m_checkTermCond(false)
{
}

Controller::~Controller()
{
}

void Controller::addModel(const t_modelptr& /*model*/)
{
	//TODO
}

void Controller::simulate()
{
	while (true) {
		//TODO
	}

}

void Controller::setClassicDEVS(bool classicDEVS)
{
	m_isClassicDEVS = classicDEVS;
}

void Controller::setTerminationTime(t_timestamp time)
{
	m_checkTermTime = true;
	m_terminationTime = time;
}

void Controller::setTerminationCondition(std::function<bool(t_timestamp, const t_modelptr&)> termination_condition)
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
