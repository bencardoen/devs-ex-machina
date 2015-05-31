/*
 * DEVStone.cpp
 *
 *  Created on: 13 Apr 2015
 *      Author: matthijs
 */

#include <performance/devstone/devstone.h>

namespace n_devstone {

/*
 * ProcessorState
 */

ProcessorState::ProcessorState()
	: State(""), m_event1_counter(-1), m_event1("")
{
}

ProcessorState::~ProcessorState()
{
}

std::string ProcessorState::toString()
{
	return m_event1;
}

std::shared_ptr<ProcessorState> ProcessorState::copy() const
{
	auto newState = n_tools::createObject<ProcessorState>();
	newState->m_event1 = m_event1;
	newState->m_event1_counter = m_event1_counter;
	newState->m_queue = m_queue;
	newState->m_state = m_state;

	return newState;
}

/*
 * Processor
 */

Processor::Processor(std::string name, bool randomta)
	: AtomicModel(name), m_randomta(randomta)
{
	addInPort("in_event1");
	addOutPort("out_event1");
	setState(n_tools::createObject<ProcessorState>());
}

Processor::~Processor()
{
}

n_model::t_timestamp Processor::timeAdvance() const
{
	auto procState =  procstate();
	if (procState.m_event1_counter == size_t(-1)) {
		return n_network::t_timestamp::infinity();
	} else {
		return n_network::t_timestamp(procState.m_event1_counter);
	}
}

void Processor::intTransition()
{
	const auto newState =  procstate().copy();
	newState->m_event1_counter = 0;
	if (newState->m_queue.empty()) {
		newState->m_event1_counter = size_t(-1);
		newState->m_event1 = "";
	} else {
		newState->m_event1 = newState->m_queue.back();
		newState->m_queue.pop_back();
		newState->m_event1_counter = (m_randomta) ? 0.75 + (rand() / (RAND_MAX / 0.5)) : 1.0;
	}
	setState(newState);
}

void Processor::extTransition(const std::vector<n_network::t_msgptr>& message)
{
	const auto newState =  procstate().copy();
	newState->m_event1_counter -= m_elapsed.getTime();

	//We can only have 1 message
	std::string ev1 = message[0]->getPayload();
	if (newState->m_event1 != "") {
		newState->m_queue.push_back(ev1);
	} else {
		newState->m_event1 = ev1;
		newState->m_event1_counter = (m_randomta) ? 0.75 + (rand() / (RAND_MAX / 0.5)) : 1.0;
	}
	setState(newState);
}

std::vector<n_network::t_msgptr> Processor::output() const
{
	const auto procState = procstate();
	auto msg = getPort("out_event1")->createMessages(procState.m_event1);
	return msg;
}

const ProcessorState& Processor::procstate() const
{
	return *(std::dynamic_pointer_cast<ProcessorState>(getState()));
}

ProcessorState& Processor::procstate()
{
	return *(std::dynamic_pointer_cast<ProcessorState>(getState()));
}



/*
 * Generator
 */

Generator::Generator() : n_model::AtomicModel("Generator")
{
	addOutPort("out_event1");
	setState(n_tools::createObject<n_model::State>("gen_event1"));
}

Generator::~Generator()
{
}

n_model::t_timestamp Generator::timeAdvance() const
{
	return n_network::t_timestamp(1);
}

void Generator::intTransition()
{
	setState(n_tools::createObject<n_model::State>("gen_event1"));
}

std::vector<n_network::t_msgptr> Generator::output() const
{
	auto msg = getPort("out_event1")->createMessages(n_tools::toString(1));
	return msg;
}

/*
 * CoupledRecursion
 */

CoupledRecursion::CoupledRecursion(std::size_t width, std::size_t depth, bool randomta)
	: CoupledModel("Coupled" + n_tools::toString(depth))
{
	n_model::t_portptr recv = addInPort("in_event1");
	n_model::t_portptr send = addOutPort("out_event1");

	if (depth > 1) {
		n_model::t_coupledmodelptr recurse = n_tools::createObject<CoupledRecursion>(width, depth - 1,
		        randomta);
		addSubModel(recurse);
		connectPorts(recv, recurse->getPort("in_event1"));

		n_model::t_atomicmodelptr prev;
		for (std::size_t i = 0; i < width; ++i) {
			n_model::t_atomicmodelptr proc = n_tools::createObject<Processor>(
			        "Processor" + n_tools::toString(depth) + "_" + n_tools::toString(i), randomta);
			addSubModel(proc);

			if (i == 0) {
				connectPorts(recurse->getPort("out_event1"), proc->getPort("in_event1"));
			} else {
				connectPorts(prev->getPort("out_event1"), proc->getPort("in_event1"));
			}
			prev = proc;
		}
		connectPorts(prev->getPort("out_event1"), send);
	} else {
		n_model::t_atomicmodelptr prev;
		for (std::size_t i = 0; i < width; ++i) {
			n_model::t_atomicmodelptr proc = n_tools::createObject<Processor>(
			        "Processor" + n_tools::toString(depth) + "_" + n_tools::toString(i), randomta);
			addSubModel(proc);

			if (i == 0) {
				connectPorts(recv, proc->getPort("in_event1"));
			} else {
				connectPorts(prev->getPort("out_event1"), proc->getPort("in_event1"));
			}
			prev = proc;
		}
		connectPorts(prev->getPort("out_event1"), send);
	}
}

CoupledRecursion::~CoupledRecursion()
{
}

/*
 * DEVStone
 */
DEVStone::DEVStone(std::size_t width, std::size_t depth, bool randomta)
	: CoupledModel("DEVStone")
{
	auto gen = n_tools::createObject<Generator>();
	auto recurse = n_tools::createObject<CoupledRecursion>(width, depth, randomta);
	addSubModel(gen);
	addSubModel(recurse);

	connectPorts(gen->getPort("out_event1"), recurse->getPort("in_event1"));

	srand (static_cast <unsigned> (time(0))); // seed once in this simulation
}

DEVStone::~DEVStone()
{
}

} /* namespace n_devstone */
