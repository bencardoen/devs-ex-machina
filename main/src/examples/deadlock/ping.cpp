/*
 * Ping.cpp
 *
 */

/**
 *      
 *      Alternate between sending / receiving messages.
 *      Sending is automated (ta=10), receiving requires a msg before going live.
 * 
 */

#include "examples/deadlock/ping.h"

namespace n_examples_deadlock {


Ping::Ping(std::string name, std::size_t priority)
	: AtomicModel<PingState>(name, priority)
{
	m_elapsed = 0;
        this->addInPort("_in");
        this->addOutPort("_out");
}

Ping::~Ping()
{
}

void Ping::extTransition(const std::vector<n_network::t_msgptr> & )
{
	PingState& st = state();
	LOG_DEBUG("MODEL :: transitioning from : " , ToString<PingState>::exec(st));
	if(st == PingState::WAITING){
	st = PingState::RECEIVING;
	return;
	}
	assert(false);
}

void Ping::intTransition()
{
	PingState& st = state();
        LOG_DEBUG("MODEL :: transitioning from : " , ToString<PingState>::exec(st));
        if(st == PingState::SENDING){
                st = PingState::WAITING;
                return;
        }
        assert(false);
}

t_timestamp Ping::timeAdvance() const
{		
	const PingState& st = state();
        if(st == PingState::SENDING)
                return t_timestamp(10);
        else if(st == PingState::WAITING)
                return t_timestamp::infinity();
        else if(st == PingState::RECEIVING)
                return t_timestamp(5);
        assert(false);
        return t_timestamp();
}

void Ping::output(std::vector<n_network::t_msgptr>& msgs) const
{
	const PingState& st = state();
        if (st == PingState::SENDING) {
                this->getPort("_out")->createMessages("i_hate_strings", msgs);
	}
}

t_timestamp Ping::lookAhead() const
{
	const PingState& st = state();
        if(st == PingState::WAITING) // Can receive message at any point, so only our current time is safe.
                return t_timestamp::epsilon();
	else if(st == PingState::RECEIVING)
                return t_timestamp(5);
	else if(st == PingState::SENDING)   // Busy for at least x time
                return t_timestamp(10);

        assert(false);
        return t_timestamp();
}

}
