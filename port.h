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
#include <memory>
#include "model.h"

namespace model {
	class Port {
	private:
		std::string m_name;
		bool m_inputPort;

		std::vector<std::shared_ptr<Port> > m_ins;
		std::map<std::shared_ptr<Port>, std::function<void> > m_outs;
		std::shared_ptr<Model> m_host;

	public:
		Port(std::string name, std::shared_ptr<Model> host, bool inputPort);

		std::string getName();
		std::string getFullName();
		bool isInPort();
		std::function<void> getZFunc(std::shared_ptr<Port> port);
		bool setZFunc(std::shared_ptr<Port> port, std::function<void> function);
		bool setInPort(std::shared_ptr<Port> port);
	};
}

#endif /* PORT_H_ */
