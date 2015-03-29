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
#include <deque>
#include <memory>
#include <sstream>
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

	t_portptr addPort(std::string name, bool isIn);

protected:
	t_timestamp m_timeLast;
	t_timestamp m_timeNext;

	t_stateptr m_state;
	std::vector<t_stateptr> m_oldStates;

	std::deque<n_network::t_msgptr> m_sendMessages;
	std::deque<n_network::t_msgptr> m_receivedMessages;

	t_portptr addInPort(std::string name);
	t_portptr addOutPort(std::string name);

public:
	Model() = delete;
	Model(std::string name);
	virtual ~Model()
	{
	}

	std::string getName() const;
	t_portptr getPort(std::string name) const;
	t_stateptr getState() const;
	void setState(const t_stateptr& newState);
	const std::map<std::string, t_portptr>& getIPorts() const;
	const std::map<std::string, t_portptr>& getOPorts() const;
	const std::deque<n_network::t_msgptr>& getSendMessages() const;
	const std::deque<n_network::t_msgptr>& getReceivedMessages() const;

};

typedef std::shared_ptr<Model> t_modelptr;
}

#endif /* MODEL_H_ */
