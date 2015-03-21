/*
 * Multicore.cpp
 *
 *  Created on: 21 Mar 2015
 *      Author: Ben Cardoen
 */

#include <multicore.h>

using namespace n_model;

Multicore::Multicore(const t_networkptr& net, std::size_t coreid /*, const t_loctableptr ltable*/)
:Core(coreid), m_network(net) /*, m_loctable(ltable */
{
}

void
Multicore::sendMessage(const t_msgptr& msg){
	// TODO fix lookup with table
	this->m_network->acceptMessage(msg);
}

void
Multicore::getMessages(std::unordered_map<std::string, std::vector<t_msgptr>>& mailbag)
{
	std::vector<t_msgptr> messages = this->m_network->getMessages(this->getCoreID());
	this->sortIncoming(mailbag, messages);
}

void
Multicore::sortIncoming(std::unordered_map<std::string, std::vector<t_msgptr>>& mailbag, const std::vector<t_msgptr>& messages)
{
	for (const auto & message : messages) {
		assert(message->getDestinationCore() == this->getCoreID());
		const std::string& destname = message->getDestinationModel();
		auto found = mailbag.find(destname);
		if (found == mailbag.end()) {
			mailbag[destname] = std::vector<t_msgptr>();
		}
		mailbag[destname].push_back(message);
		}
}
