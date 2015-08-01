/*
 * firegenerator.cpp
 *
 *  Created on: May 4, 2015
 *      Author: Stijn Manhaeve - Devs Ex Machina
 */

#include "examples/forestfire/firegenerator.h"
#include "examples/forestfire/constants.h"
#include "tools/objectfactory.h"

namespace n_examples {

FireGeneratorState::FireGeneratorState(): m_status(true)
{
}

std::string FireGeneratorState::toString()
{
	if(m_status)
		return "GeneratorState: true";
	return "GeneratorState: false";
}

FireGenerator::FireGenerator(std::size_t levels):
	AtomicModel("Generator")
{
	setState(n_tools::createObject<FireGeneratorState>());
	m_outputs.reserve(levels);
	for(std::size_t i = 0; i < levels; ++i) {
		std::stringstream ssr;
		ssr << "out_" << i;
		m_outputs.push_back(addOutPort(ssr.str()));
	}
}

void FireGenerator::intTransition()
{
	t_fgstateptr newState = n_tools::createObject<FireGeneratorState>();
	newState->m_status = false;
	setState(newState);
}

n_network::t_timestamp FireGenerator::timeAdvance() const
{
	const FireGeneratorState& state = getFGState();
	if(state.m_status)
		return n_network::t_timestamp(1);
	return n_network::t_timestamp::infinity();
}

FireGeneratorState& FireGenerator::getFGState()
{
	return *(std::dynamic_pointer_cast<FireGeneratorState>(getState()));
}

const FireGeneratorState& FireGenerator::getFGState() const
{
	return *(std::dynamic_pointer_cast<FireGeneratorState>(getState()));
}

std::vector<n_network::t_msgptr> FireGenerator::output() const
{
	std::vector<n_network::t_msgptr> container;
	container.reserve(m_oPorts.size());
	double i = 1.0;
	for(const std::map<std::string, n_model::t_portptr>::value_type& port : m_oPorts){
		double val = T_AMBIENT + T_GENERATE/i;
		port.second->createMessages(val, container);
		i *= 2.0;
	}
	for(n_network::t_msgptr& ptr: container){
		LOG_DEBUG("created message: ", ptr->toString());
	}
	return container;
}

} /* namespace n_examples */

std::vector<n_model::t_portptr>& n_examples::FireGenerator::getOutputs()
{
	return m_outputs;
}
