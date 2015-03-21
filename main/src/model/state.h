/*
 * state.h
 *
 *  Created on: 9-mrt.-2015
 *      Author: Pieter, Tim
 */

#ifndef STATE_H_
#define STATE_H_

namespace n_model {

using n_network::t_timestamp;

class State
{
public:
	t_timestamp m_timeLast;
	t_timestamp m_timeNext;

	std::string m_state;

	State(std::string state) {m_state = state;}

	virtual std::string toString() {return m_state;};
	virtual std::string toXML() = 0;
	virtual std::string toJSON() = 0;
	virtual std::string toCell() = 0;
	virtual ~State() {}
};

typedef std::shared_ptr<State> t_stateptr;
}

#endif /* STATE_H_ */
