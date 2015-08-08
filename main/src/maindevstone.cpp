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
	std::size_t m_curn;
	std::size_t m_coren;
public:
	DevstoneAlloc(std::size_t elemNum, std::size_t corenum): m_maxn(elemNum), m_curn(0), m_coren(corenum){

	}
	virtual size_t allocate(n_model::t_atomicmodelptr& ptr){
		std::size_t res = 0;
		if(m_curn > (m_maxn/m_coren)*m_coren){
			res = m_coren-1u;
		} else {
			res = m_curn*m_coren/m_maxn;
		}
		LOG_INFO("Putting model ", ptr->getName(), " in core ", res);
		++m_curn;
		return res;
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
	conf.name = "DEVStone";
	conf.simType = simType;
	if (type == "cpdevs")
		conf.pdevsType = n_control::ControllerConfig::CONSERVATIVE;
	conf.coreAmount = coreAmt;
	conf.saveInterval = 5;
	conf.zombieIdleThreshold = 10;
	conf.allocator = n_tools::createObject<DevstoneAlloc>(width*depth + 1u, coreAmt);

	std::ofstream filestream("./devstone.txt");
	{
		CoutRedirect myRedirect(filestream);
		auto ctrl = conf.createController();
		t_timestamp endTime(100000, 0);
		ctrl->setTerminationTime(endTime);

		t_coupledmodelptr d = n_tools::createObject< n_devstone::DEVStone>(width, depth, false);
		ctrl->addModel(d);

		ctrl->simulate();
	}
}
