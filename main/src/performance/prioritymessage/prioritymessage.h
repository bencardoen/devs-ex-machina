/*
 * prioritymessage.h
 *
 *  Created on: Nov 18, 2015
 *      Author: lttlemoi
 */

#ifndef SRC_PERFORMANCE_PRIORITYMESSAGE_PRIORITYMESSAGE_H_
#define SRC_PERFORMANCE_PRIORITYMESSAGE_PRIORITYMESSAGE_H_

#include "control/allocator.h"
#include "model/state.h"
#include "model/atomicmodel.h"
#include "model/coupledmodel.h"
#include <random>
#include <sstream>
#include <vector>

namespace n_priorityMsg {

struct PriorityState
{
        std::size_t m_count; //number of messages generated
        std::size_t m_toSendCount; //number of priority messages to send this round
        bool m_sendPriority;
        n_network::t_timestamp::t_time m_ttl;

        PriorityState()
                : m_count(0), m_toSendCount(0), m_sendPriority(0), m_ttl(0)
        {
        }
};

typedef std::ranlux48 t_randgen;

class Generator: public n_model::AtomicModel<PriorityState>
{
private:
        const std::size_t m_numMessages;
        const std::size_t m_priorityChance;
        mutable t_randgen m_rand;
        mutable std::uniform_int_distribution<std::size_t> m_dist;

public:
        std::vector<n_model::t_portptr> m_outs;
        Generator(std::string name, std::size_t numMessages, std::size_t numOuts, std::size_t priority);
        virtual ~Generator();

        virtual n_model::t_timestamp timeAdvance() const;
        virtual void intTransition();
        virtual void extTransition(const std::vector<n_network::t_msgptr> & message);
        virtual void confTransition(const std::vector<n_network::t_msgptr> & message);
        virtual void output(std::vector<n_network::t_msgptr>& msgs) const;
        virtual n_network::t_timestamp lookAhead() const;

};

class Receiver: public n_model::AtomicModel<void>
{
public:
        n_model::t_portptr m_in;
        Receiver(std::string name);
        virtual ~Receiver();

        virtual n_model::t_timestamp timeAdvance() const;
        virtual void intTransition();
        virtual void extTransition(const std::vector<n_network::t_msgptr> & message);
        virtual void confTransition(const std::vector<n_network::t_msgptr> & message);
        virtual void output(std::vector<n_network::t_msgptr>& msgs) const;
        virtual n_network::t_timestamp lookAhead() const;

};

class PriorityMessage: public n_model::CoupledModel
{
public:
        /**
         * @param numMessages     Number of normal messages
         * @param numReceivers     The number of receivers
         * @param priority  Chance of generating a priority message
         */
        PriorityMessage(std::size_t numMessages, std::size_t numReceivers, std::size_t priority);
        virtual ~PriorityMessage(){}
};

} /* namespace n_priorityMsg */

template<>
struct ToString<n_priorityMsg::PriorityState>
{
        static std::string exec(const n_priorityMsg::PriorityState& s)
        {
                std::stringstream ssr;
                ssr << "c: " << s.m_count << ", p:" << s.m_sendPriority;
                return ssr.str();
        }
};

#endif /* SRC_PERFORMANCE_PRIORITYMESSAGE_PRIORITYMESSAGE_H_ */
