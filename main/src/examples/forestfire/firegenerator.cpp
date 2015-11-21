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

FireGenerator::FireGenerator(std::size_t levels):
	AtomicModel<FGState>("Generator", FGState(true))
{
	m_outputs.reserve(levels);
	for(std::size_t i = 0; i < levels; ++i) {
		std::stringstream ssr;
		ssr << "out_" << i;
		m_outputs.push_back(addOutPort(ssr.str()));
	}
}

void FireGenerator::intTransition()
{
	state().m_value = false;
}

n_network::t_timestamp FireGenerator::timeAdvance() const
{
	if(state().m_value)
		return n_network::t_timestamp(1);
	return n_network::t_timestamp::infinity();
}

void FireGenerator::output(std::vector<n_network::t_msgptr>& msgs) const
{
	double i = 1.0;
	for(const n_model::t_portptr& port : m_oPorts){
		double val = T_AMBIENT + T_GENERATE/i;
		port->createMessages(val, msgs);
		i *= 2.0;
	}
}

std::vector<n_model::t_portptr>& FireGenerator::getOutputs()
{
	return m_outputs;
}

} /* namespace n_examples */
