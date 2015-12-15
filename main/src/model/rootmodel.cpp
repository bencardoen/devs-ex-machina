/*
 * This file is part of the DEVS Ex Machina project.
 * Copyright 2014 - 2015 University of Antwerp 
 * https://www.uantwerpen.be/en/
 * Licensed under the EUPL V.1.1
 * A full copy of the license is in COPYING.txt, or can be found at 
 * https://joinup.ec.europa.eu/community/eupl/og_page/eupl 
 *      Author: Stijn Manhaeve, Ben Cardoen, Tim Tuijn, Pieter Lauwers, Matthijs Van Os
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
	std::deque<CoupledModel*> toDo;
	toDo.push_back(model.get());
	while (!toDo.empty()) {
		CoupledModel* top = toDo.front();
		toDo.pop_front();
		for (t_modelptr& current : top->getComponents()) {
			t_atomicmodelptr atomic = std::dynamic_pointer_cast<AtomicModel_impl>(current);
			if (atomic) {
				m_components.push_back(atomic);
			} else {
				toDo.push_back(n_tools::staticRawCast<CoupledModel>(current.get()));
			}
		}
	}
}

const std::vector<t_atomicmodelptr>& n_model::RootModel::directConnect(const t_coupledmodelptr& model)
{
	if (m_directConnected)
		return m_components;
	m_components.clear();
//	find all atomic models
	{
		std::deque<t_coupledmodelptr> toDo;
		toDo.push_back(model);
		while (!toDo.empty()) {
			t_coupledmodelptr top = toDo.front();
			toDo.pop_front();
			for (t_modelptr& current : top->getComponents()) {
				t_atomicmodelptr atomic = std::dynamic_pointer_cast<AtomicModel_impl>(current);
				if (atomic) { //model = atomic
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

		record(const t_portptr_raw out, const t_portptr_raw in, t_zfunc zfunc)
			: out(out), in(in), zfunc(zfunc)
		{
		}
	};
	{
		std::deque<record> worklistOut;
		std::deque<t_portptr_raw> worklistIn;
		// loop over all atomic models
		for (t_atomicmodelptr& atomic : m_components) {
			//loop over all its output ports
			LOG_INFO("DIRCON: Direct connecting model ", atomic->getName());
			for (t_portptr& out : atomic->getOPorts()) {
				LOG_INFO("DIRCON: Direct connecting outport ", out->getName());
				//reset any previous data for direct connect
				out->resetDirectConnect();
				out->setUsingDirectConnect(true);
				//loop over all outgoing connections
				//get all outgoing connections
				for (const t_outconnect& link : out->getOuts()) {
					LOG_INFO("DIRCON: Worklist: ", out->getName(), " => ",
					        link.first->getName());
					worklistOut.emplace_back(out.get(), link.first, link.second);
				}
				//do the direct connect
				while (!worklistOut.empty()) {
					record rec = worklistOut.front();
					worklistOut.pop_front();
					if (dynamic_cast<AtomicModel_impl*>(rec.in->getHost())) {
						//link to atomic model, make the direct connection
						LOG_INFO("DIRCON: Linking ", rec.out->getName(), "[OUT] to ",
						        rec.in->getName(), "[IN]");
						rec.out->setZFuncCoupled(rec.in, rec.zfunc);
					} else {
						//this is a link to a port of a coupled devs!
						//loop over its connected ports & squash the links together
						for (const t_outconnect& link2 : rec.in->getOuts()) {

							t_zfunc replacement = nullptr;

							if (rec.zfunc) {
								if (link2.second) {
									// zfunc of receiver exists
									// zfunc of link2 exists
									// thus we make the combo
									replacement = n_tools::createObject<ZFuncCombo>(rec.zfunc, link2.second);
								} else {								
									// zfunc of receiver exists
									// zfunc of link2 does not
									replacement = rec.zfunc;
								}
							} else {
								// zfunc of receiver does not exist
								// use link2 (either nullptr or existing zfunc)
								replacement = link2.second;
							}

							worklistOut.emplace_back(rec.out, link2.first,
							        replacement);
						}
					}
				}
			}
			//loop over all the input ports
			for (t_portptr& in : atomic->getIPorts()) {
				LOG_INFO("DIRCON: Direct connecting inport ", in->getName());
				in->resetDirectConnect();
				in->setUsingDirectConnect(true);

				//get all incoming connections
				std::vector<t_portptr_raw>& ins = in->getIns();
				worklistIn.insert(worklistIn.end(), ins.begin(), ins.end());
				LOG_INFO("DIRCON: Direct connecting inport ", in->getName(), " -> got all incoming connections: ", worklistIn.size());
				//do the direct connect
				while (!worklistIn.empty()) {
					LOG_INFO("DIRCON: direct connect incoming worklist size: ", worklistIn.size());
					t_portptr_raw rec = worklistIn.front();
					worklistIn.pop_front();
					if (dynamic_cast<AtomicModel_impl*>(rec->getHost())) {
						//link to atomic model, make the direct connection
						LOG_INFO("DIRCON: Linking ", in->getName(), "[IN] from ",
						        rec->getName(), "[OUT]");
						in->setInPortCoupled(rec);
					} else {
						//this is a link to a port of a coupled devs!
						LOG_INFO("DIRCON: direct connect incoming, processing port from coupled: ", rec->getName());
						//loop over its connected ports & squash the links together
						std::vector<t_portptr_raw>& ins2 = rec->getIns();
						worklistIn.insert(worklistIn.end(), ins2.begin(), ins2.end());
					}
				}
			}
		}

	}

	return m_components;
}

void RootModel::reset()
{
	m_directConnected = false;
	m_components.clear();
}

} /* namespace n_model */
