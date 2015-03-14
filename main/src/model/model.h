/*
 * model.h
 *
 *  Created on: 9-mrt.-2015
 *      Author: Pieter, Tim
 */

#ifndef MODEL_H_
#define MODEL_H_

#include <string>
#include <map>
#include <vector>
#include <memory>
#include "port.h"
#include "state.h"

namespace n_model {

using n_network::t_timestamp;

class Model
{
private:
	std::string m_name;

	std::map<std::string, t_portptr> m_iPorts;
	std::map<std::string, t_portptr> m_oPorts;

	bool m_receivedExt;
	t_timestamp m_timeLast;
	t_timestamp m_timeNext;
	unsigned int m_location;

	t_stateptr m_state;
	std::vector<t_stateptr> m_oldStates;

	t_portptr addPort(std::string name, bool isIn);

protected:
	Model(std::string name);

	t_portptr addInPort(std::string);
	t_portptr addOutPort(std::string);

public:
	std::string getName() const;
	t_stateptr getState() const;
	void setState(t_stateptr newState);
};
}

#endif /* MODEL_H_ */
