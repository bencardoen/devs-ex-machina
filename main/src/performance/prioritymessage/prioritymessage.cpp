/*
 * This file is part of the DEVS Ex Machina project.
 * Copyright 2014 - 2015 University of Antwerp
 * https://www.uantwerpen.be/en/
 * Licensed under the EUPL V.1.1
 * A full copy of the license is in COPYING.txt, or can be found at
 * https://joinup.ec.europa.eu/community/eupl/og_page/eupl
 *      Author: Stijn Manhaeve, Ben Cardoen
 */

#include "performance/prioritymessage/prioritymessage.h"
#include "tools/objectfactory.h"
#include "tools/stringtools.h"

#ifdef FPTIME
#define T_MIN 0.01
#define T_MAX 1.0
#define T_LARGE 100.0
#else
#define T_MIN 1
#define T_MAX 100
#define T_LARGE 10000
#endif

n_priorityMsg::Generator::Generator(std::string name, std::size_t numMessages, std::size_t numOuts,
        std::size_t priority)
        : n_model::AtomicModel<PriorityState>(name), m_numMessages(numMessages), m_priorityChance(priority),
          m_dist(0, numOuts-1)
{
        setCorenumber(0);
        m_outs.reserve(numOuts);
        for (std::size_t i = 0; i < numOuts; ++i) {
                m_outs.push_back(addOutPort(std::string("out_") + n_tools::toString(i)));
        }
}

n_priorityMsg::Generator::~Generator()
{
}

n_model::t_timestamp n_priorityMsg::Generator::timeAdvance() const
{
        if (state().m_sendPriority) {
                return T_MIN;
        }
        return n_network::t_timestamp(
                std::size_t((getTimeLast().getTime() / T_MAX) + 1) * T_MAX - getTimeLast().getTime());
}

void n_priorityMsg::Generator::intTransition()
{
        n_priorityMsg::PriorityState& st = state();
        if (st.m_sendPriority) {
                st.m_sendPriority = false;
                st.m_count += m_numMessages;
        } else {
                std::uniform_int_distribution<std::size_t> dist(0, 100);
                st.m_sendPriority = m_priorityChance < dist(m_rand);
                st.m_toSendCount = 1u;
                st.m_count += (st.m_sendPriority ? 1 : m_numMessages);
        }
}

void n_priorityMsg::Generator::extTransition(const std::vector<n_network::t_msgptr>& /*message*/)
{
        // nothing to do here
}

void n_priorityMsg::Generator::confTransition(const std::vector<n_network::t_msgptr>& /*message*/)
{
        // nothing to do here
}

void n_priorityMsg::Generator::output(std::vector<n_network::t_msgptr>& msgs) const
{
        std::size_t rSeed = state().m_count;
        m_rand.seed(rSeed);
        if (state().m_sendPriority) {
                for (std::size_t i = 0; i < state().m_toSendCount; ++i) {
                        //get random port
                        m_outs[m_dist(m_rand)]->createMessages<bool>(true, msgs);
                }
        } else {
                for (std::size_t i = 0; i < m_numMessages; ++i) {
                        //get random port
                        m_outs[m_dist(m_rand)]->createMessages<bool>(true, msgs);
                }
        }
}

n_network::t_timestamp n_priorityMsg::Generator::lookAhead() const
{
        return timeAdvance();
}

n_priorityMsg::Receiver::Receiver(std::string name)
        : n_model::AtomicModel<void>(name), m_in(addInPort("in"))
{
        setCorenumber(1);
}

n_priorityMsg::Receiver::~Receiver()
{
}

n_model::t_timestamp n_priorityMsg::Receiver::timeAdvance() const
{
        return n_network::t_timestamp(
                std::size_t((getTimeLast().getTime() / T_LARGE) + 1) * T_LARGE - getTimeLast().getTime());
}

void n_priorityMsg::Receiver::intTransition()
{
}

void n_priorityMsg::Receiver::extTransition(const std::vector<n_network::t_msgptr>&)
{
}

void n_priorityMsg::Receiver::confTransition(const std::vector<n_network::t_msgptr>&)
{
}

void n_priorityMsg::Receiver::output(std::vector<n_network::t_msgptr>&) const
{
}

n_network::t_timestamp n_priorityMsg::Receiver::lookAhead() const
{
        return T_MIN;
}

n_priorityMsg::PriorityMessage::PriorityMessage(std::size_t numMessages, std::size_t numReceivers, std::size_t priority)
        : n_model::CoupledModel("PriorityMessage")
{
        auto gen = n_tools::createObject<Generator>("Generator", numMessages, numReceivers, priority);
        addSubModel(gen);
        gen->setCorenumber(0u);
        for (std::size_t i = 0; i < numReceivers; ++i) {
                auto rec = n_tools::createObject<Receiver>("Receiver" + n_tools::toString(i));
                addSubModel(std::static_pointer_cast<n_model::t_atomicmodelptr::element_type>(rec));
                connectPorts(gen->m_outs[i], rec->m_in);
                rec->setCorenumber(1u);
        }
}
