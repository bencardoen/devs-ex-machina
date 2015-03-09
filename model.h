/*
 * model.h
 *
 *  Created on: 9-mrt.-2015
 *      Author: Pieter
 */

#ifndef MODEL_H_
#define MODEL_H_

#include <string>
#include <map>
#include <vector>
#include <memory>
#include "port.h"

namespace model {
	typedef double t_time;

	class Model {
	private:
		std::string m_name;

		std::map<std::string, std::shared_ptr<Port>> m_iPorts;
		std::map<std::string, std::shared_ptr<Port>> m_oPorts;

		bool m_receivedExt;
		t_time m_timeLast;
		t_time m_timeNext;
		unsigned int m_location;

		std::shared_ptr<State> m_state;
		std::vector<std::shared_ptr<State>> m_oldStates;

		std::shared_ptr<Port> addPort(std::string name, bool isIn);

	protected:
		Model(std::string name);

		std::shared_ptr<Port> addInPort(std::string);
		std::shared_ptr<Port> addOutPort(std::string);

	public:
		std::string getName();
		std::shared_ptr<State> getState();
		void setState(std::shared_ptr<State> newState);
	};
}

#endif /* MODEL_H_ */
