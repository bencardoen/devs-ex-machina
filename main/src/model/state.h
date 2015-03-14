/*
 * state.h
 *
 *  Created on: 9-mrt.-2015
 *      Author: Pieter, Tim
 */

#ifndef STATE_H_
#define STATE_H_

namespace n_model {
class State
{
	virtual std::string toString();
	virtual std::string toXML();
	virtual std::string toJSON();
	virtual std::string toCell();
	virtual ~State();
};

typedef std::shared_ptr<State> t_stateptr;
}

#endif /* STATE_H_ */
