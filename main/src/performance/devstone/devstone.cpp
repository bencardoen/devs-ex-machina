/*
 * This file is part of the DEVS Ex Machina project.
 * Copyright 2014 - 2015 University of Antwerp
 * https://www.uantwerpen.be/en/
 * Licensed under the EUPL V.1.1
 * A full copy of the license is in COPYING.txt, or can be found at
 * https://joinup.ec.europa.eu/community/eupl/og_page/eupl
 *      Author: Stijn Manhaeve, Ben Cardoen, Tim Tuijn, Matthijs Van Os
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

constexpr t_counter inf = std::numeric_limits<t_counter>::max();

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

Processor::Processor(std::string name, bool randomta, std::size_t num)
	: AtomicModel<ProcessorState>(name), m_randomta(randomta), m_out(addOutPort("out_event1")), m_num(num)
{
	addInPort("in_event1");
	LOG_DEBUG("Created devstone processor ", name, " with number ", m_num);
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
    bool replacedMessage = false;
	if (st.m_queue.empty()) {
		st.m_event1_counter = inf;
		st.m_event1 = Event(0u);
	} else {
		st.m_event1 = st.m_queue.back();
		st.m_queue.pop_back();
		st.m_event1_counter = (m_randomta) ? getProcTime(st.m_event1) : T_100;
        replacedMessage = true;
	}
	++(st.m_eventsHad);
	Event ev1 = n_network::getMsgPayload<Event>(message[0]);
	if (st.m_event1 != Event(0) || replacedMessage) {
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
#ifdef __CYGWIN__
	return round(val/gran)*gran;
#else
	return std::round(val/gran)*gran;
#endif
}

t_counter Processor::getProcTime(size_t event) const
{
#ifdef FPTIME
	std::uniform_real_distribution<t_counter> dist(T_75, T_125);
	m_rand.seed((event + m_num + state().m_eventsHad)*m_num);
	return roundTo(dist(m_rand), T_STEP);
#else
	std::uniform_int_distribution<t_counter> dist(T_75, T_125);
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

CoupledRecursion::CoupledRecursion(std::size_t width, std::size_t depth, bool randomta, std::size_t& num)
	: CoupledModel("Coupled" + n_tools::toString(depth))
{
	//set two outer ports
    n_model::t_portptr recv = addInPort("in_event1");
    n_model::t_portptr send = addOutPort("out_event1");

    //create the lower object.
    if(depth > 1){
        LOG_INFO("  depth > 1!");
        n_model::t_coupledmodelptr prev = n_tools::createObject<CoupledRecursion>(1, depth - 1, randomta, num);
        addSubModel(prev);
        connectPorts(recv, prev->getIPorts()[0]);

        for(std::size_t i = 1; i < width; ++i){
            n_model::t_coupledmodelptr proc = n_tools::createObject<CoupledRecursion>(1, depth - 1, randomta, num);
            addSubModel(proc);
            connectPorts(prev->getOPorts()[0], proc->getIPorts()[0]);
            prev = proc;
        }

        //connect end of line with higher level.
        connectPorts(prev->getOPorts()[0], send);
    }

    //create the list of processors and link them up
    else {
        n_model::t_atomicmodelptr prev = n_tools::createObject<Processor>(
                                        "Processor_" + n_tools::toString(num), randomta, num);
        num++;
        addSubModel(prev);
        connectPorts(recv, prev->getIPorts()[0]);

        for(std::size_t i = 1; i < width; ++i){
            n_model::t_atomicmodelptr proc = n_tools::createObject<Processor>(
                                "Processor_" + n_tools::toString(num), randomta, num);
            num++;
            addSubModel(proc);
            connectPorts(prev->getOPorts()[0], proc->getIPorts()[0]);
            prev = proc;
        }

        //connect end of line with higher level.
        connectPorts(prev->getOPorts()[0], send);
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
	std::size_t num = 0u;
	auto recurse = n_tools::createObject<CoupledRecursion>(width, depth, randomta, num);
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
