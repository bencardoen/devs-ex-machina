/*
 * rootmodel.cpp
 *
 *  Created on: 30 Mar 2015
 *      Author: matthijs
 */
#include "model/rootmodel.h"
#include <deque>
#include <set>
#include <cassert>

namespace n_model {

RootModel::RootModel()
	: m_directConnected(false)
{
}

void RootModel::setComponents(const t_coupledmodelptr& model)
{
	std::deque<t_coupledmodelptr> toDo;
	toDo.push_back(model);
	while (!toDo.empty()) {
		t_coupledmodelptr top = toDo.front();
		toDo.pop_front();
		for (t_modelptr& current : top->getComponents()) {
			current->setParent(top);
			t_atomicmodelptr atomic = std::dynamic_pointer_cast<AtomicModel_impl>(current);
			if (atomic) {
				m_components.push_back(atomic);
			} else {
				toDo.push_back(std::static_pointer_cast<CoupledModel>(current));
			}
		}
	}
}

const std::vector<t_atomicmodelptr>& n_model::RootModel::directConnect(const t_coupledmodelptr& model)
{
	if (m_directConnected)
		return m_components;
	m_components.clear();

	std::set<std::string> atomics;
//	find all atomic models
	{
		std::deque<t_coupledmodelptr> toDo;
		toDo.push_back(model);
		while (!toDo.empty()) {
			t_coupledmodelptr top = toDo.front();
			toDo.pop_front();
			for (t_modelptr& current : top->getComponents()) {
				current->setParent(top);
				t_atomicmodelptr atomic = std::dynamic_pointer_cast<AtomicModel_impl>(current);
				if (atomic) {
					//model = atomic
					auto res = atomics.insert(atomic->getName());
					assert(res.second && "All atomic models should have a unique name.");
					m_components.push_back(atomic);
				} else {
					toDo.push_back(std::static_pointer_cast<CoupledModel>(current));
				}
			}
		}
	}
//	create all direct connections
	// keeps track of all the connections that we still have to do
	struct record
	{
		const t_portptr_raw out;	//outgoing port of the connection
		const t_portptr_raw in;
		const t_zfunc zfunc;

		record(const t_portptr_raw out, const t_portptr_raw in, const t_zfunc& zfunc)
			: out(out), in(in), zfunc(zfunc)
		{
		}
	};
	{
		// loop over all atomic models
		for (t_atomicmodelptr& atomic : m_components) {
			//loop over all its output ports
			LOG_INFO("DIRCON: Direct connecting model ", atomic->getName());
			for (std::pair<const std::string, t_portptr>& out : atomic->getOPorts()) {
				LOG_INFO("DIRCON: Direct connecting outport ", out.second->getName());
				//reset any previous data for direct connect
				out.second->resetDirectConnect();
				out.second->setUsingDirectConnect(true);
				//loop over all outgoing connections
				std::deque<record> worklist;
				//get all outgoing connections
				for (const t_outconnect& link : out.second->getOuts()) {
					LOG_INFO("DIRCON: Worklist: ", out.second->getFullName(), " => ",
					        link.first->getFullName());
					worklist.emplace_back(out.second.get(), link.first, link.second);
				}
				//do the direct connect
				while (!worklist.empty()) {
					record rec = worklist.front();
					worklist.pop_front();
					if (atomics.find(rec.in->getHostName()) != atomics.end()) {
						//link to atomic model, make the direct connection
						LOG_INFO("DIRCON: Linking ", rec.out->getFullName(), "[OUT] to ",
						        rec.in->getFullName(), "[IN]");
						rec.out->setZFuncCoupled(rec.in, rec.zfunc);
					} else {
						//this is a link to a port of a coupled devs!
						//loop over its connected ports & squash the links together
						for (t_outconnect link2 : rec.in->getOuts()) {
							worklist.emplace_back(rec.out, link2.first,
							        n_tools::createObject<ZFuncCombo>(rec.zfunc,
							                link2.second));
						}
					}
				}
			}
			//loop over all the input ports
			for (std::pair<const std::string, t_portptr>& in : atomic->getIPorts()) {
				LOG_INFO("DIRCON: Direct connecting inport ", in.second->getName());
				in.second->resetDirectConnect();
				in.second->setUsingDirectConnect(true);
				std::deque<t_portptr_raw> worklist;

				//get all incoming connections
				std::vector<t_portptr_raw>& ins = in.second->getIns();
				worklist.insert(worklist.end(), ins.begin(), ins.end());
				LOG_INFO("DIRCON: Direct connecting inport ", in.second->getName(), " -> got all incoming connections: ", worklist.size());
				//do the direct connect
				while (!worklist.empty()) {
					LOG_INFO("DIRCON: direct connect incoming worklist size: ", worklist.size());
					t_portptr_raw rec = worklist.front();
					worklist.pop_front();
					if (atomics.find(rec->getHostName()) != atomics.end()) {
						//link to atomic model, make the direct connection
						LOG_INFO("DIRCON: Linking ", in.second->getFullName(), "[IN] from ",
						        rec->getFullName(), "[OUT]");
						in.second->setInPortCoupled(rec);
					} else {
						//this is a link to a port of a coupled devs!
						LOG_INFO("DIRCON: direct connect incoming, processing port from coupled: ", rec->getFullName());
						//loop over its connected ports & squash the links together
						std::vector<t_portptr_raw>& ins2 = rec->getIns();
						worklist.insert(worklist.end(), ins2.begin(), ins2.end());
					}
				}
			}
		}

	}

	return m_components;
}

void RootModel::undoDirectConnect()
{
	m_directConnected = false;
	LOG_DEBUG("undid direct connect");
}

std::vector<t_atomicmodelptr> RootModel::getComponents()
{
	return m_components;
}

} /* namespace n_model */
