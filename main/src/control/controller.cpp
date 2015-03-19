/*
 * Controller.cpp
 *
 *  Created on: 11 Mar 2015
 *      Author: matthijs
 */

#include "controller.h"

namespace n_control {

Controller::Controller(std::string name, std::unordered_map<std::size_t, t_coreptr> cores)
	: m_isClassicDEVS(true), m_name(name), m_checkTermTime(false), m_checkTermCond(false), m_cores(cores)
{
}

Controller::~Controller()
{
}

void Controller::addModel(const t_atomicmodelptr& atomic)
{
	//FIXME dumb implementation
	std::size_t core = 0;
	addModel(atomic, core);
}

void Controller::addModel(const t_atomicmodelptr& atomic, std::size_t coreID)
{
	m_cores[coreID]->addModel(atomic);
	locTab->registerModel(atomic, coreID);
}

void Controller::addModel(const t_coupledmodelptr& coupled)
{
	std::vector<t_atomicmodelptr> atomics = directConnect(coupled);
	for (auto at : atomics) {
		addModel(at);
	}
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

void Controller::setTerminationCondition(
        std::function<bool(t_timestamp, const t_atomicmodelptr&)> termination_condition)
{
	m_checkTermCond = true;
	m_terminationCondition = termination_condition;
}

bool Controller::check()
{
	if (m_checkTermTime) {
		//TODO
	}
	if (m_checkTermCond) {
		//TODO
	}
	return false;
}

std::vector<t_atomicmodelptr> Controller::directConnect(const t_coupledmodelptr& coupled)
{
//	std::vector<t_atomicmodelptr> components = coupled->getComponents(); // MOVE

	std::vector<t_atomicmodelptr> connected; // FIXME Implement direct connect

	return connected;
}

} /* namespace n_control */
