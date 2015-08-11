/*
 * DEVStone.cpp
 *
 *  Created on: 13 Apr 2015
 *      Author: matthijs
 */

#include "performance/devstone/devstone.h"
#include "tools/stringtools.h"
#include <limits>

#ifdef FPTIME
#define T_0 0.0
#define T_50 0.5
#define T_75 0.75
#define T_100 1.0
#else
#define T_0 0u
#define T_50 50u
#define T_75 75u
#define T_100 100u
#endif



namespace n_devstone {
const t_counter inf = std::numeric_limits<t_counter>::max();

/*
 * ProcessorState
 */

ProcessorState::ProcessorState()
	: State(""), m_event1_counter(inf), m_event1(0)
{
}

ProcessorState::~ProcessorState()
{
}

std::string ProcessorState::toString()
{
	return n_tools::toString(m_event1);
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

std::size_t Processor::m_numcounter = 1;

Processor::Processor(std::string name, bool randomta)
	: AtomicModel(name), m_randomta(randomta), m_out(addOutPort("out_event1")), m_num(m_numcounter++)
{
	addInPort("in_event1");
	setState(n_tools::createObject<ProcessorState>());
	LOG_DEBUG("Created devstone processor ", name);
}

Processor::~Processor()
{
}

n_model::t_timestamp Processor::timeAdvance() const
{
	const auto& procState = procstate();
	if (procState.m_event1_counter == inf) {
		return n_network::t_timestamp::infinity();
	} else {
		return n_network::t_timestamp(procState.m_event1_counter);
	}
}

void Processor::intTransition()
{
	const auto newState = procstate().copy();
	newState->m_event1_counter = T_0;
	if (newState->m_queue.empty()) {
		newState->m_event1_counter = inf;
		newState->m_event1 = Event(0u);
	} else {
		newState->m_event1 = newState->m_queue.back();
		newState->m_queue.pop_back();
		newState->m_event1_counter = (m_randomta) ? (T_75 + (rand() / (RAND_MAX / T_50))) : T_100;
	}
	LOG_DEBUG("internal event counter of ", getName(), " = ", newState->m_event1_counter, " =inf ", newState->m_event1_counter == inf);
	setState(newState);
}

void Processor::extTransition(const std::vector<n_network::t_msgptr>& message)
{
	const auto newState = procstate().copy();
	LOG_DEBUG("externala event counter of ", getName(), " = ", newState->m_event1_counter, ", time elapsed: ", m_elapsed, " =inf ", newState->m_event1_counter == inf);
	newState->m_event1_counter = (newState->m_event1_counter == inf)? inf: (newState->m_event1_counter - m_elapsed.getTime());
	LOG_DEBUG("externalb event counter of ", getName(), ", new value = ", newState->m_event1_counter);
	//We can only have 1 message
	Event ev1 = n_network::getMsgPayload<Event>(message[0]);
	if (newState->m_event1 != Event(0)) {
		newState->m_queue.push_back(ev1);
	} else {
		newState->m_event1 = ev1;
		//TODO devstone randomized counter float
		newState->m_event1_counter = (m_randomta) ? (T_75 + (rand() / (RAND_MAX / T_50))) : T_100;
	}
	LOG_DEBUG("confluent event counter of ", getName(), " = ", newState->m_event1_counter, " =inf ", newState->m_event1_counter == inf);
	setState(newState);
}

void Processor::confTransition(const std::vector<n_network::t_msgptr>& message)
{
	const auto newState = procstate().copy();
	newState->m_event1_counter = T_0;
	LOG_DEBUG("event counter of ", getName(), " = ", newState->m_event1_counter, ", time elapsed: ", m_elapsed);
	//We can only have 1 message
	if (newState->m_queue.empty()) {
		newState->m_event1_counter = inf;
		newState->m_event1 = Event(0u);
	} else {
		newState->m_event1 = newState->m_queue.back();
		newState->m_queue.pop_back();
		newState->m_event1_counter = (m_randomta) ? T_75 + (rand() / (RAND_MAX / T_50)) : T_100;
	}
	Event ev1 = n_network::getMsgPayload<Event>(message[0]);
	if (newState->m_event1 != Event(0)) {
		newState->m_queue.push_back(ev1);
	} else {
		newState->m_event1 = ev1;
		//TODO devstone randomized counter float
		newState->m_event1_counter = (m_randomta) ? T_75 + (rand() / (RAND_MAX / T_50)) : T_100;
	}
	setState(newState);
}

std::vector<n_network::t_msgptr> Processor::output() const
{
	const auto& procState = procstate();
	return m_out->createMessages(procState.m_event1);
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

Generator::Generator() : n_model::AtomicModel("Generator"), m_out(addOutPort("out_event1"))
{
	setState(n_tools::createObject<n_model::State>("gen_event1"));
}

Generator::~Generator()
{
}

n_model::t_timestamp Generator::timeAdvance() const
{
	return n_network::t_timestamp(T_100);
}

void Generator::intTransition()
{
	setState(n_tools::createObject<n_model::State>("gen_event1"));
}

std::vector<n_network::t_msgptr> Generator::output() const
{
	return m_out->createMessages(Event(100));
}

/*
 * CoupledRecursion
 */

CoupledRecursion::CoupledRecursion(std::size_t width, std::size_t depth, bool randomta)
	: CoupledModel("Coupled" + n_tools::toString(depth))
{
	// If possible, split layers (CoupledRecursion) over cores in even chunks
	// eg. depth 7, coreAmt 2 -> core#0: layer 1,2,3,4 core#1: layer 5,6,7
//	int location = (depth>1 && coreAmt!=-1) ? (depth-1) / std::max(int(totalDepth/coreAmt + totalDepth%2), 1) : -1;

	//set own ports
	n_model::t_portptr recv = addInPort("in_event1");
	n_model::t_portptr send = addOutPort("out_event1");

	//create the lower object.
	n_model::t_coupledmodelptr recurse = nullptr;
	if(depth > 1){
		LOG_INFO("  depth > 1!");
		recurse = n_tools::createObject<CoupledRecursion>(width, depth - 1, randomta);
		addSubModel(recurse);
		connectPorts(recv, recurse->getPort("in_event1"));
	}

	//create the list of processors and link them up
	n_model::t_atomicmodelptr prev = nullptr;
	for(std::size_t i = 0; i < width; ++i){
		n_model::t_atomicmodelptr proc = n_tools::createObject<Processor>(
					        "Processor" + n_tools::toString(depth) + "_" + n_tools::toString(i), randomta);
		addSubModel(proc);
		if(i == 0){
			if(depth > 1){
				connectPorts(recurse->getPort("out_event1"), proc->getPort("in_event1"));
			} else {
				connectPorts(recv, proc->getPort("in_event1"));
			}
		} else {
			connectPorts(prev->getPort("out_event1"), proc->getPort("in_event1"));
		}
		prev = proc;
	}

	//connect end of line with higher level.
	connectPorts(prev->getPort("out_event1"), send);
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
