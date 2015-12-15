/*
 * This file is part of the DEVS Ex Machina project.
 * Copyright 2014 - 2015 University of Antwerp
 * https://www.uantwerpen.be/en/
 * Licensed under the EUPL V.1.1
 * A full copy of the license is in COPYING.txt, or can be found at
 * https://joinup.ec.europa.eu/community/eupl/og_page/eupl
 *      Author: Stijn Manhaeve, Tim Tuijn
 */

#include "test/testmodels.h"
#include "tools/stringtools.h"
#include <sstream>
#include <limits>

namespace n_testmodel
{

std::ostream& operator <<(std::ostream& stream, const Event& event)
{
	stream << "EventSize = " << event.m_eventSize;
	return stream;
}

ModelState::ModelState():
	m_counter(std::numeric_limits<std::size_t>::max()),
	m_event{0}
{
}

Processor::Processor(std::string name, std::size_t t_event1):
	AtomicModel<ModelState>(name),
	m_event1(t_event1),
	m_inport(addInPort("inport")),
	m_outport(addOutPort("outport"))
{
}

void Processor::extTransition(const std::vector<n_network::t_msgptr>& message)
{
	if(message.empty())
		return;
	const Event& event = n_network::getMsgPayload<Event>(message[0]);

	state().m_counter = m_event1;
	state().m_event = event;
}

void Processor::intTransition()
{
	state() = ModelState();
}

n_network::t_timestamp Processor::timeAdvance() const
{
	return state().m_counter;
}

void Processor::output(std::vector<n_network::t_msgptr>& msgs) const
{
	m_outport->createMessages(state().m_event, msgs);
}

GeneratorState::GeneratorState():
	ModelState(),
	m_generated(0u),
	m_value(1u)
{
}

Generator::Generator(std::string name, std::size_t t_gen_event1, bool binary):
	AtomicModel<GeneratorState>(name),
	m_gen_event1(t_gen_event1),
	m_binary(binary),
	m_inport(addInPort("inport")),
	m_outport(addOutPort("outport"))
{
	state().m_counter = t_gen_event1;
}

void Generator::extTransition(const std::vector<n_network::t_msgptr>&)
{
	state().m_counter -= getTimeElapsed().getTime();
}

void Generator::intTransition()
{
	++(state().m_generated);
}

n_network::t_timestamp Generator::timeAdvance() const
{
	return state().m_counter;
}

void Generator::output(std::vector<n_network::t_msgptr>& msgs) const
{
	if(m_binary){
		m_outport->createMessages("b1", msgs);
		return;
	}
	Event e = {state().m_value};
	m_outport->createMessages(e, msgs);
}


CoupledProcessor::CoupledProcessor(std::size_t event1_P1, std::size_t levels):
	CoupledModel(std::string("CoupledProcessor_") + n_tools::toString(levels)),
	m_inport(addInPort("inport")),
	m_outport(addOutPort("outport"))
{
	for(std::size_t i = 0; i < levels; ++i)
		addSubModel(n_tools::createObject<Processor>(std::string("Processor") + n_tools::toString(i), event1_P1));
	for(std::size_t i = 0; i < levels-1; ++i)
		connectPorts(std::dynamic_pointer_cast<Processor>(m_components[i])->m_outport, std::dynamic_pointer_cast<Processor>(m_components[i+1])->m_inport);
	connectPorts(m_inport, std::dynamic_pointer_cast<Processor>(m_components[0])->m_inport);
	connectPorts(std::dynamic_pointer_cast<Processor>(m_components[levels - 1u])->m_outport, m_outport);
}

ElapsedNothing::ElapsedNothing():
	AtomicModel<ModelState>("ElapsedNothing")
{
	m_elapsed = 3u;
	state().m_counter = 10;
}

void ElapsedNothing::intTransition()
{
	state() = ModelState();
	state().m_counter = 0;
}

n_network::t_timestamp ElapsedNothing::timeAdvance() const
{
	std::size_t i = state().m_counter;
	if(i)
		return i;
	return n_network::t_timestamp::infinity();
}

GeneratorDS::GeneratorDS():
	Generator("GEN")
{
	m_elapsed = 5u;
}

void GeneratorDS::output(std::vector<n_network::t_msgptr>& msgs) const
{
	if(state().m_generated < 1)
		Generator::output(msgs);
}

bool GeneratorDS::modelTransition(n_model::DSSharedState*)
{
	if(state().m_generated == 1){
		removePort(m_outport);
		m_outport.reset();
		return true;
	}
	return false;
}

DSDevsRoot::DSDevsRoot():
	CoupledModel("Root"),
	m_model(n_tools::createObject<GeneratorDS>()),
	m_model2(n_tools::createObject<Processor>("Processor_2")),
	m_model3(n_tools::createObject<Processor>("Processor_3")),
	m_model4(nullptr),
	m_modelX(nullptr)
{
	addSubModel(m_model);
	addSubModel(m_model2);
	addSubModel(m_model3);
	connectPorts(m_model->m_outport, m_model2->m_inport);
	connectPorts(m_model2->m_outport, m_model3->m_inport);
	connectPorts(m_model->m_outport, m_model3->m_inport);
}

bool DSDevsRoot::modelTransition(n_model::DSSharedState*)
{
	n_model::t_modelptr ptr = std::dynamic_pointer_cast<n_model::Model>(m_model2);
	removeSubModel(ptr);
	m_model2 = n_tools::createObject<Processor>("Processor_2");
	addSubModel(m_model2);
	connectPorts(m_model2->m_outport, m_model3->m_inport);
	m_model4 = n_tools::createObject<CoupledProcessor>(2u, 3);
	addSubModel(m_model4);
	connectPorts(m_model3->m_outport, m_model4->m_inport);
	m_modelX = n_tools::createObject<ElapsedNothing>();
	addSubModel(m_modelX);

	return false;
}
} /* namespace n_testmodel */
