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

namespace n_model {

typedef std::function<n_network::t_msgptr(const n_network::t_msgptr&)> t_zfunc;

class Port
{
private:
	std::string m_name;
	std::string m_hostname;
	bool m_inputPort;

	std::vector<std::shared_ptr<Port> > m_ins;
	std::map<std::shared_ptr<Port>, t_zfunc> m_outs;

public:
	Port(std::string name, std::string hostname, bool inputPort);

	std::string getName() const;
	std::string getFullName() const;
	bool isInPort() const;
	std::function<void(const n_network::t_msgptr&)> getZFunc(std::shared_ptr<Port> port) const;
	bool setZFunc(std::shared_ptr<Port> port, t_zfunc function = t_zfunc());
	bool setInPort(std::shared_ptr<Port> port);
};

typedef std::shared_ptr<Port> t_portptr;
}

#endif /* PORT_H_ */
