/*
 * maindevstone.cpp
 *
 *      Author: Matthijs Van Os - Devs Ex Machina
 */

#include "control/controllerconfig.h"
#include "tools/coutredirect.h"
#include "performance/devstone/devstone.h"
#include "tools/stringtools.h"
#include "control/allocator.h"

LOG_INIT("devstone.log")

class DevstoneAlloc: public n_control::Allocator
{
private:
	std::size_t m_maxn;
	std::size_t m_coren;
public:
	DevstoneAlloc(std::size_t elemNum, std::size_t corenum): m_maxn(elemNum), m_coren(corenum){

	}
	virtual size_t allocate(const n_model::t_atomicmodelptr& ptr){
		auto p = std::dynamic_pointer_cast<n_devstone::Processor>(ptr);
		if(p == nullptr) return 0;
		std::size_t res = 0;
		std::size_t curn = p->m_num;
//		if(curn > (m_maxn/m_coren)*m_coren){
//			res = m_coren-1u;
//		} else {
			res = curn*m_coren/m_maxn;
//		}
		if(res >= m_coren) res = m_coren-1;
		LOG_INFO("Putting model ", ptr->getName(), " in core ", res);
		return res;
	}

	virtual void allocateAll(const std::vector<n_model::t_atomicmodelptr>& models){
		for(const n_model::t_atomicmodelptr& ptr: models)
			ptr->setCorenumber(allocate(ptr));
	}
};

/**
 * The executable takes up to 4 arguments (in this order):
 * - The type of simulation:
 * 	+ "classic" 	- Classic DEVS
 * 	+ "pdevs" 	- Optimistic Parallel DEVS
 * 	+ "opdevs"	- Optimistic Parallel DEVS
 * 	+ "cpdevs"	- Conservative Parallel DEVS
 * 	-> If you choose for a parallel simulation, your next argument should specify the amount of cores!
 * - The width (amount of processors per coupled model)
 * - The depth (amount of linked coupled models)
 */
int main(int argc, char** args)
{
	// default values:
	std::string type;
	n_control::Controller::SimType simType = n_control::Controller::CLASSIC;
	int widthpos = 2;
	int depthpos = 3;
	int coreAmt = 1;
	std::size_t width = 2;
	std::size_t depth = 3;

	if(argc >= 2) {
		type = args[1];
		if(type == "pdevs" || type == "opdevs" || type == "cpdevs") {
			simType = n_control::Controller::PDEVS;
			if(argc >= 3)
				coreAmt = n_tools::toInt(args[2]);
				++depthpos;
				++widthpos;
		}
	}
	if(argc >= widthpos+1) {
		width = n_tools::toInt(args[widthpos]);
	}
	if(argc >= depthpos+1) {
		depth = n_tools::toInt(args[depthpos]);
	}

	n_control::ControllerConfig conf;
	conf.m_name = "DEVStone";
	conf.m_simType = simType;
	if (type == "cpdevs")
		conf.m_pdevsType = n_control::ControllerConfig::CONSERVATIVE;
	conf.m_coreAmount = coreAmt;
	conf.m_saveInterval = 5;
	conf.m_zombieIdleThreshold = 10;
	conf.m_allocator = n_tools::createObject<DevstoneAlloc>(width*depth + 1u, coreAmt);

	std::ofstream filestream("./devstone.txt");
	{
		n_tools::CoutRedirect myRedirect(filestream);
		auto ctrl = conf.createController();
		t_timestamp endTime(100000, 0);
		ctrl->setTerminationTime(endTime);

		t_coupledmodelptr d = n_tools::createObject< n_devstone::DEVStone>(width, depth, false);
		ctrl->addModel(d);

		ctrl->simulate();
	}
}
