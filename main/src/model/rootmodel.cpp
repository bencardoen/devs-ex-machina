/*
 * rootmodel.cpp
 *
 *  Created on: 30 Mar 2015
 *      Author: matthijs
 */
#include "rootmodel.h"
#include <deque>
#include <set>
#include <cassert>

namespace n_model {

n_model::RootModel::RootModel()
	: Model("_ROOT"), m_directConnected(false)
{
}

n_model::RootModel::~RootModel()
{
}

const std::vector<t_atomicmodelptr>& n_model::RootModel::directConnect(t_coupledmodelptr& model)
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
			t_coupledmodelptr& top = toDo.front();
			toDo.pop_front();
			for (t_modelptr& current : top->getComponents()) {
				t_atomicmodelptr atomic = std::dynamic_pointer_cast<AtomicModel>(current);
				if (atomic) {
					//model = atomic
					auto res = atomics.insert(atomic->getName());
					assert(res.second && "All atomic models should have a unique name.");
					m_components.push_back(atomic);
				} else {
					toDo.push_back(std::dynamic_pointer_cast<CoupledModel>(current));
				}
			}
		}
	}
//	create all direct connections
	// keeps track of all the connections that we still have to do
	struct record
	{
		t_portptr out;	//outgoing port of the connection
		t_portptr in;
		t_zfunc zfunc;

		record(t_portptr out, t_portptr in, t_zfunc zfunc)
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
				for (std::pair<t_portptr, t_zfunc> link : out.second->getOuts()) {
					LOG_INFO("DIRCON: Worklist: ", out.second->getFullName(), " => ",
					        link.first->getFullName());
					worklist.emplace_back(out.second, link.first, link.second);
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
						for (std::pair<t_portptr, t_zfunc> link2 : rec.in->getOuts()) {
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
				std::deque<t_portptr> worklist;

				//get all incoming connections
				for (t_portptr& link : in.second->getIns()) {
					worklist.push_back(link);
				}
				//do the direct connect
				while (!worklist.empty()) {
					t_portptr rec = worklist.front();
					worklist.pop_front();
					if (atomics.find(rec->getHostName()) != atomics.end()) {
						//link to atomic model, make the direct connection
						LOG_INFO("DIRCON: Linking ", in.second->getFullName(), "[IN] from ",
						        rec->getFullName(), "[OUT]");
						in.second->setInPortCoupled(rec);
					} else {
						//this is a link to a port of a coupled devs!
						//loop over its connected ports & squash the links together
						for (t_portptr link2 : rec->getIns()) {
							worklist.push_back(link2);
						}
					}
				}
			}
		}

	}

	return m_components;
}

void n_model::RootModel::undoDirectConnect()
{
	m_directConnected = false;
}

} /* namespace n_model */
