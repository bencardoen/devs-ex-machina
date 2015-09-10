/*
 * Ping.cpp
 *
 */

/**
 *      PX_out -> PXX_in
 *      P has two states alternating :
 *              Processing : P has received a message and is busy for a set amount
 *              of time. After that time, P goes to the next state
 *              For lookahead debugging, add void internal states in this range.
 *              Communicating : P will generate and receive output. On receipt, it will
 *              go to the state processing.
 *
 */

#include "examples/deadlock/ping.h"

namespace n_examples_deadlock {

Ping::Ping(std::string name, std::size_t priority)
	: AtomicModel(name, priority)
{
        this->setState(n_tools::createObject<State>("processing"));
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
    if(*state == "communicating"){
        this->setState("processing");
        return;
    }
    assert(false);
    LOG_DEBUG("MODEL :: transitioning from : " , state->toString());
}

void Ping::intTransition()
{
	t_stateptr state = this->getState();
        LOG_DEBUG("MODEL :: transitioning from : " , state->toString());
        if(*state == "processing"){
                this->setState("communicating");
                return;
        }
        assert(false);
	return;
}

t_timestamp Ping::timeAdvance() const
{		
    if(*(this->getState()) == "processing")
        return t_timestamp(10);
    else
        return t_timestamp::infinity();
}

void Ping::output(std::vector<n_network::t_msgptr>& msgs) const
{
	t_stateptr state = this->getState();
        if ((*state=="communicating")) {
                this->getPort("_out")->createMessages("i_hate_strings", msgs);
	}
}

t_timestamp Ping::lookAhead() const
{
	t_stateptr state = this->getState();
        if(*state == "communicating"){ // Can receive message at any point, so only our current time is safe.
                return t_timestamp::epsilon();
        }
        if(*state == "processing"){   // Busy for at least x time
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
