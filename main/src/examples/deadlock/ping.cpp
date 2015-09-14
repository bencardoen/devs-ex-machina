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
	: AtomicModel_impl(name, priority)
{
        this->setState(n_tools::createObject<State>("sending"));
	m_elapsed = 0;
        this->addInPort("_in");
        this->addOutPort("_out");
}

Ping::~Ping()
{
}

void Ping::extTransition(const std::vector<n_network::t_msgptr> & )
{
    t_stateptr state = this->getState();
    LOG_DEBUG("MODEL :: transitioning from : " , state->toString());
    if(*state == "waiting"){
        this->setState("receiving");
        return;
    }
    assert(false);
}

void Ping::intTransition()
{
	t_stateptr state = this->getState();
        LOG_DEBUG("MODEL :: transitioning from : " , state->toString());
        if(*state == "sending"){
                this->setState("waiting");
                return;
        }
        assert(false);
	return;
}

t_timestamp Ping::timeAdvance() const
{		
        auto state = this->getState();
        if(*state == "sending")
                return t_timestamp(10);
        if(*state == "waiting")
                return t_timestamp::infinity();
        if(*state == "receiving")
                return t_timestamp(5);
        assert(false);
}

void Ping::output(std::vector<n_network::t_msgptr>& msgs) const
{
	t_stateptr state = this->getState();
        if ((*state=="sending")) {
                this->getPort("_out")->createMessages("i_hate_strings", msgs);
	}
}

t_timestamp Ping::lookAhead() const
{
	t_stateptr state = this->getState();
        if(*state == "waiting"){ // Can receive message at any point, so only our current time is safe.
                return t_timestamp::epsilon();
        }
        if(*state == "receiving")
                return t_timestamp(5);
        if(*state == "sending"){   // Busy for at least x time
                return t_timestamp(10);
        }
        assert(false);
}

t_stateptr Ping::setState(std::string s)
{
	this->Model::setState(n_tools::createObject<State>(s));
	return this->getState();
}

}
