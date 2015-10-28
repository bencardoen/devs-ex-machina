/*
 * DEVStone.cpp
 *
 *  Created on: 13 Apr 2015
 *      Author: matthijs
 */

#include "performance/devstone/devstone.h"
#include "tools/stringtools.h"
#include <limits>
#include <random>

#ifdef FPTIME
#define T_0 0.0
#define T_1 0.01
#define T_50 0.5
#define T_75 0.75
#define T_100 1.0
#define T_125 1.25
#define T_STEP 0.01
#else
#define T_0 0u
#define T_1 1u
#define T_50 50u
#define T_75 75u
#define T_100 100u
#define T_125 125u
#endif



namespace n_devstone {

const t_counter inf = std::numeric_limits<t_counter>::max();

/*
 * ProcessorState
 */
ProcessorState::ProcessorState():
	m_event1_counter(inf), m_event1(0), m_eventsHad(0)
{
}

/*
 * Processor
 */

std::size_t Processor::m_numcounter = 1;

Processor::Processor(std::string name, bool randomta)
	: AtomicModel<ProcessorState>(name), m_randomta(randomta), m_out(addOutPort("out_event1")), m_num(m_numcounter++)
{
	addInPort("in_event1");
	LOG_DEBUG("Created devstone processor ", name);
}

Processor::~Processor()
{
}

n_model::t_timestamp Processor::timeAdvance() const
{
	const ProcessorState& st = state();
	if (st.m_event1_counter == inf) {
		return n_network::t_timestamp::infinity();
	} else {
		return n_network::t_timestamp(st.m_event1_counter);
	}
}

void Processor::intTransition()
{
	ProcessorState& st = state();
	if (st.m_queue.empty()) {
		st.m_event1_counter = inf;
		st.m_event1 = Event(0u);
	} else {
		st.m_event1 = st.m_queue.back();
		st.m_queue.pop_back();
		st.m_event1_counter = (m_randomta) ? getProcTime(st.m_event1) : T_100;
	}
	++(st.m_eventsHad);
	LOG_DEBUG("internal event counter of ", getName(), " = ", st.m_event1_counter, " =inf ", st.m_event1_counter == inf);
}

void Processor::extTransition(const std::vector<n_network::t_msgptr>& message)
{
	ProcessorState& st = state();
	LOG_DEBUG("externala event counter of ", getName(), " = ", st.m_event1_counter, ", time elapsed: ", m_elapsed, " =inf ", st.m_event1_counter == inf);
	st.m_event1_counter = (st.m_event1_counter == inf)? inf: (st.m_event1_counter - m_elapsed.getTime());
	LOG_DEBUG("externalb event counter of ", getName(), ", new value = ", st.m_event1_counter);
	//We can only have 1 message
	Event ev1 = n_network::getMsgPayload<Event>(message[0]);
	if (st.m_event1 != Event(0)) {
		st.m_queue.push_back(ev1);
	} else {
		st.m_event1 = ev1;
		st.m_event1_counter = (m_randomta) ? getProcTime(st.m_event1)  : T_100;
	}
	LOG_DEBUG("confluent event counter of ", getName(), " = ", st.m_event1_counter, " =inf ", st.m_event1_counter == inf);
}

void Processor::confTransition(const std::vector<n_network::t_msgptr>& message)
{
	ProcessorState& st = state();
	LOG_DEBUG("event counter of ", getName(), " = ", st.m_event1_counter, ", time elapsed: ", m_elapsed);
	//We can only have 1 message
	if (st.m_queue.empty()) {
		st.m_event1_counter = inf;
		st.m_event1 = Event(0u);
	} else {
		st.m_event1 = st.m_queue.back();
		st.m_queue.pop_back();
		st.m_event1_counter = (m_randomta) ? getProcTime(st.m_event1) : T_100;
	}
	++(st.m_eventsHad);
	Event ev1 = n_network::getMsgPayload<Event>(message[0]);
	if (st.m_event1 != Event(0)) {
		st.m_queue.push_back(ev1);
	} else {
		st.m_event1 = ev1;
		st.m_event1_counter = (m_randomta) ? getProcTime(st.m_event1)  : T_100;
	}
}

void Processor::output(std::vector<n_network::t_msgptr>& msgs) const
{
	m_out->createMessages(state().m_event1, msgs);
}

n_network::t_timestamp Processor::lookAhead() const
{
	if(m_randomta)
		return n_network::t_timestamp(T_1);
        else{
		return n_network::t_timestamp(T_100);
        }
}

template<typename T>
constexpr T roundTo(T val, T gran)
{
	return std::round(val/gran)*gran;
}

t_counter Processor::getProcTime(size_t event) const
{
#ifdef FPTIME
	static std::uniform_real_distribution<t_counter> dist(T_75, T_125);
	m_rand.seed((event + m_num + state().m_eventsHad)*m_num);
	return roundTo(dist(m_rand), T_STEP);
#else
	static std::uniform_int_distribution<t_counter> dist(T_75, T_125);
	m_rand.seed((event + m_num + state().m_eventsHad)*m_num);
	return dist(m_rand);
#endif
}

/*
 * Generator
 */

Generator::Generator() : n_model::AtomicModel<void>("Generator"), m_out(addOutPort("out_event1"))
{
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
}

void Generator::output(std::vector<n_network::t_msgptr>& msgs) const
{
	return m_out->createMessages(Event(100), msgs);
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
		connectPorts(recv, recurse->getIPorts()[0]);
	}

	//create the list of processors and link them up
	n_model::t_atomicmodelptr prev = nullptr;
	for(std::size_t i = 0; i < width; ++i){
		n_model::t_atomicmodelptr proc = n_tools::createObject<Processor>(
					        "Processor" + n_tools::toString(depth) + "_" + n_tools::toString(i), randomta);
		addSubModel(proc);
		if(i == 0){
			if(depth > 1){
				connectPorts(recurse->getOPorts()[0], proc->getIPorts()[0]);
			} else {
				connectPorts(recv, proc->getIPorts()[0]);
			}
		} else {
			connectPorts(prev->getOPorts()[0], proc->getIPorts()[0]);
		}
		prev = proc;
	}

	//connect end of line with higher level.
	connectPorts(prev->getOPorts()[0], send);
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

	connectPorts(gen->m_out, recurse->getIPorts()[0]);
}

DEVStone::~DEVStone()
{
}

n_network::t_timestamp Generator::lookAhead() const
{
	return n_network::t_timestamp::infinity();
}

} /* namespace n_devstone */
