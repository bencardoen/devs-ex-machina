/*
 * port.h
 *
 *  Created on: 9-mrt.-2015
 *      Author: Pieter
 */

#ifndef PORT_H_
#define PORT_H_

#include <vector>
#include <map>
#include <algorithm>
#include "message.h"
#include "zfunc.h"

namespace n_model {

class Port
{
private:
	std::string m_name;
	std::string m_hostname;
	bool m_inputPort;

	std::vector<std::shared_ptr<Port> > m_ins;
	std::map<std::shared_ptr<Port>, t_zfunc> m_outs;

	std::map<std::shared_ptr<Port>, t_zfunc> m_coupled_outs;

	bool m_usingDirectConnect;

public:
	Port(std::string name, std::string hostname, bool inputPort);

	std::string getName() const;
	std::string getFullName() const;
	std::string getHostName() const;
	bool isInPort() const;
	t_zfunc getZFunc(const std::shared_ptr<Port>& port) const;
	bool setZFunc(const std::shared_ptr<Port>& port, t_zfunc function);
	bool setInPort(const std::shared_ptr<Port>& port);

	bool setZFuncCoupled(const std::shared_ptr<Port>& port, t_zfunc function);
	void setUsingDirectConnect(bool dc);

	std::vector<n_network::t_msgptr> createMessages(std::string message);
};

typedef std::shared_ptr<Port> t_portptr;
}

#endif /* PORT_H_ */
