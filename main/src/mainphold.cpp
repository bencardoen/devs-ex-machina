/*
 * mainphold.cpp
 *
 *      Author: Devs Ex Machina
 */

#include <performance/phold/phold.h>
#include "control/controllerconfig.h"
#include "tools/coutredirect.h"

LOG_INIT("phold.log")


// The problem with phold is that it will always generate a lot reverts
// this is the reason why classic, unparallel simulation will be a lot faster than parallel.
class PHoldAlloc: public n_control::Allocator
{
private:
	std::size_t m_maxn;
	std::size_t m_n;
public:
	PHoldAlloc(): m_maxn(0), m_n(0)
	{

	}
	virtual size_t allocate(const n_model::t_atomicmodelptr&){
		//all models may send to each other. There isn't really an optimal configuration.
		return (m_n++)%coreAmount();
	}

	virtual void allocateAll(const std::vector<n_model::t_atomicmodelptr>& models){
		m_maxn = models.size();
		assert(m_maxn && "Total amount of models can't be zero.");
		for(const n_model::t_atomicmodelptr& ptr: models){
			std::size_t val = allocate(ptr);
			ptr->setCorenumber(val);
			LOG_DEBUG("Assigning model ", ptr->getName(), " to core ", val);
		}
	}
};



template<typename T>
T toData(std::string str)
{
	T num;
	std::istringstream ss(str);
	ss >> num;
	return num;
}

char getOpt(char* argv){
	if(strlen(argv) == 2 && argv[0] == '-')
		return argv[1];
	return 0;
}

const char helpstr[] = " [-h] [-t ENDTIME] [-n NODES] [-s SUBNODES] [-r REMOTES] [-i ITER] [-c COREAMT] [classic|cpdevs|opdevs|pdevs]\n"
	"options:\n"
	"  -h             show help and exit\n"
	"  -t ENDTIME     set the endtime of the simulation\n"
	"  -n NODES       number of phold nodes\n"
	"  -s SUBNODES    number of subnodes per phold node\n"
	"  -r REMOTES     percentage of remote connections\n"
	"  -i ITER        amount of useless work to simulate complex calculations\n"
	"  -c COREAMT     amount of simulation cores, ignored in classic mode\n"
	"  classic        Run single core simulation.\n"
	"  cpdevs         Run conservative parallel simulation.\n"
	"  opdevs|pdevs   Run optimistic parallel simulation.\n"
	"note:\n"
	"  If the same option is set multiple times, only the last value is taken.\n";
int main(int argc, char** argv)
{
	const char optETime = 't';
	const char optWidth = 'n';
	const char optDepth = 's';
	const char optHelp = 'h';
	const char optIter = 'i';
	const char optRemote = 'r';
	const char optCores = 'c';
	char** argvc = argv+1;

#ifdef FPTIME
	n_network::t_timestamp::t_time eTime = 50.0;
#else
	n_network::t_timestamp::t_time eTime = 50;
#endif
	std::size_t nodes = 1;
	std::size_t apn = 10;
	std::size_t percentageRemotes = 10;
	std::size_t iter = 0;

	bool hasError = false;
	n_control::SimType simType = n_control::SimType::CLASSIC;
	std::size_t coreAmt = 4;

	for(int i = 1; i < argc; ++argvc, ++i){
		char c = getOpt(*argvc);
		if(!c){
			if(!strcmp(*argvc, "classic")){
				simType = n_control::SimType::CLASSIC;
				continue;
			} else if(!strcmp(*argvc, "cpdevs")){
				simType = n_control::SimType::CONSERVATIVE;
				continue;
			} else if(!strcmp(*argvc, "opdevs") || !strcmp(*argvc, "pdevs")){
				simType = n_control::SimType::OPTIMISTIC;
				continue;
			} else {
				std::cout << "Unknown argument: " << *argvc << '\n';
				hasError = true;
				continue;
			}
		}
		switch(c){
		case optCores:
			++i;
			if(i < argc){
				coreAmt = toData<std::size_t>(std::string(*(++argvc)));
				if(coreAmt == 0){
					std::cout << "Invalid argument for option -" << optCores << '\n';
					hasError = true;
				}
			} else {
				std::cout << "Missing argument for option -" << optCores << '\n';
			}
			break;
		case optETime:
			++i;
			if(i < argc){
				eTime = toData<n_network::t_timestamp::t_time>(std::string(*(++argvc)));
			} else {
				std::cout << "Missing argument for option -" << optETime << '\n';
			}
			break;
		case optWidth:
			++i;
			if(i < argc){
				nodes = toData<std::size_t>(std::string(*(++argvc)));
			} else {
				std::cout << "Missing argument for option -" << optWidth << '\n';
			}
			break;
		case optDepth:
			++i;
			if(i < argc){
				apn = toData<std::size_t>(std::string(*(++argvc)));
                                if(apn < 2){
                                        std::cerr << "Invalid depth size in phold, min==2\n";
                                        hasError = true;
                                }
			} else {
				std::cout << "Missing argument for option -" << optDepth << '\n';
			}
			break;
		case optRemote:
			++i;
			if(i < argc){
				percentageRemotes = toData<std::size_t>(std::string(*(++argvc)));
			} else {
				std::cout << "Missing argument for option -" << optRemote << '\n';
			}
			break;
		case optIter:
			++i;
			if(i < argc){
				iter = toData<std::size_t>(std::string(*(++argvc)));
			} else {
				std::cout << "Missing argument for option -" << optIter << '\n';
			}
			break;
		case optHelp:
			std::cout << "usage: \n\t" << argv[0] << helpstr;
			return 0;
		default:
			std::cout << "Unknown argument: " << *argvc << '\n';
			hasError = true;
			continue;
		}
	}
	if(hasError){
		std::cout << "usage: \n\t" << argv[0] << helpstr;
		return -1;
	}

	n_control::ControllerConfig conf;
	conf.m_name = "DEVStone";
	conf.m_simType = simType;
	conf.m_coreAmount = coreAmt;
	conf.m_saveInterval = 5;
	conf.m_zombieIdleThreshold = 10;
	conf.m_allocator = n_tools::createObject<PHoldAlloc>();

	auto ctrl = conf.createController();
	t_timestamp endTime(eTime, 0);
	ctrl->setTerminationTime(endTime);

	t_coupledmodelptr d = n_tools::createObject<n_benchmarks_phold::PHOLD>(nodes, apn, iter,
	        percentageRemotes);
	ctrl->addModel(d);
	{
#ifndef BENCHMARK
		std::ofstream filestream("./phold.txt");
		n_tools::CoutRedirect myRedirect(filestream);
#endif /* BENCHMARK */

		ctrl->simulate();
	}
#ifdef USE_VIZ
        ctrl->visualize();
#endif        
        
#ifdef USE_STAT
	ctrl->printStats(std::cout);
	d->printStats(std::cout);
#endif
}
