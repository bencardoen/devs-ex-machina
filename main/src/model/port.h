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
#include "globallog.h"

namespace n_model {

class Port;

typedef std::shared_ptr<Port> t_portptr;

class Port
{
private:
	std::string m_name;
	std::string m_hostname;
	bool m_inputPort;

	std::vector<t_portptr > m_ins;
	std::map<t_portptr, t_zfunc> m_outs;

	std::map<t_portptr, std::vector<t_zfunc>> m_coupled_outs;
	std::vector<t_portptr> m_coupled_ins;

	bool m_usingDirectConnect;

public:
	Port(std::string name, std::string hostname, bool inputPort);

	std::string getName() const;
	std::string getFullName() const;
	std::string getHostName() const;
	bool isInPort() const;
	t_zfunc getZFunc(const t_portptr& port) const;
	bool setZFunc(const t_portptr& port, t_zfunc function);
	bool setInPort(const t_portptr& port);
	void clear();
	/**
	 * @brief Remove a connection from this port to the other
	 */
	void removeOutPort(const t_portptr& port);
	void removeInPort(const t_portptr& port);

	void setZFuncCoupled(const t_portptr& port, t_zfunc function);
	void setInPortCoupled(const t_portptr& port);
	void setUsingDirectConnect(bool dc);
	void resetDirectConnect();
	bool isUsingDirectConnect() const;

	std::vector<n_network::t_msgptr> createMessages(std::string message);
	const std::vector<t_portptr >& getIns() const;
	const std::map<t_portptr, t_zfunc>& getOuts() const;
	std::vector<t_portptr >& getIns();
	std::map<t_portptr, t_zfunc>& getOuts();
	const std::vector<t_portptr>& getCoupledIns() const;
	const std::map<t_portptr, std::vector<t_zfunc> >& getCoupledOuts() const;
};

}

#endif /* PORT_H_ */
