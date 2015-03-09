/*
 * state.h
 *
 *  Created on: 9-mrt.-2015
 *      Author: Pieter
 */

#ifndef STATE_H_
#define STATE_H_

namespace model {
	class State {
		virtual std::string toString();
		virtual std::string toXML();
		virtual std::string toJSON();
		virtual std::string toCell();
		virtual ~State();
	};
}

#endif /* STATE_H_ */
