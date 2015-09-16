/*
 * maininterconnect.cpp
 *
 *      Author: Devs Ex Machina
 */

#include "control/controllerconfig.h"
#include "tools/coutredirect.h"
#include "performance/highInterconnect/hinterconnect.h"
#include "tools/stringtools.h"
#include "control/allocator.h"

#include "tools/statistic.h"

LOG_INIT("interconnect.log")

class InterconnectAlloc: public n_control::Allocator
{
private:
	std::size_t m_maxn;
public:
	InterconnectAlloc(): m_maxn(0){

	}
	virtual size_t allocate(const n_model::t_atomicmodelptr&){
		return 0;
	}

	virtual void allocateAll(const std::vector<n_model::t_atomicmodelptr>& models){
		m_maxn = models.size();
		assert(m_maxn && "Total amount of models can't be zero.");
		std::size_t i = 0;
		for(const n_model::t_atomicmodelptr& ptr: models)
			ptr->setCorenumber((i++)%coreAmount());
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

/**
 * cmd args:
 * [-h] [-t ENDTIME] [-w WIDTH][-r] [-c COREAMT] [classic|cpdevs|opdevs|pdevs]
 * 	-h: show help and exit
 * 	-t ENDTIME set the endtime of the simulation
 * 	-w WIDTH the with of the devstone model
 * 	-c COREAMT amount of simulation cores, ignored in classic mode
 * 	classic run single core simulation
 * 	cpdevs run conservative parallel simulation
 * 	opdevs|pdevs run optimistic parallel simulation
 * The last value entered for an option will overwrite any previous values for that option.
 */
const char helpstr[] = " [-h] [-t ENDTIME] [-w WIDTH] [-r] [-c COREAMT] [classic|cpdevs|opdevs|pdevs]\n"
	"options:\n"
	"  -h           show help and exit\n"
	"  -t ENDTIME   set the endtime of the simulation\n"
	"  -w WIDTH     the width of the high interconnect model\n"
	"  -r           use randomized processing time\n"
	"  -c COREAMT   amount of simulation cores, ignored in classic mode. Must not be 0.\n"
	"  classic      Run single core simulation.\n"
	"  cpdevs       Run conservative parallel simulation.\n"
	"  opdevs|pdevs Run optimistic parallel simulation.\n"
	"note:\n"
	"  If the same option is set multiple times, only the last value is taken.\n";

int main(int argc, char** argv)
{
	// default values:
	const char optETime = 't';
	const char optWidth = 'w';
	const char optHelp = 'h';
	const char optRand = 'r';
	const char optCores = 'c';
	char** argvc = argv+1;

#ifdef FPTIME
	n_network::t_timestamp::t_time eTime = 50.0;
#else
	n_network::t_timestamp::t_time eTime = 50;
#endif
	std::size_t width = 2;
	bool randTa = false;

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
				width = toData<std::size_t>(std::string(*(++argvc)));
			} else {
				std::cout << "Missing argument for option -" << optWidth << '\n';
			}
			break;
		case optRand:
			randTa = true;
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
	conf.m_name = "High Interconnect";
	conf.m_simType = simType;
	conf.m_coreAmount = coreAmt;
	conf.m_saveInterval = 5;     
	conf.m_zombieIdleThreshold = 10;
	conf.m_allocator = n_tools::createObject<InterconnectAlloc>();

	auto ctrl = conf.createController();
	t_timestamp endTime(eTime, 0);
	ctrl->setTerminationTime(endTime);

	t_coupledmodelptr d = n_tools::createObject<n_interconnect::HighInterconnect>(width, randTa);
        
	ctrl->addModel(d);
	{
#ifndef BENCHMARK
		std::ofstream filestream("./interconnect.txt");
		n_tools::CoutRedirect myRedirect(filestream);
#endif /* BENCHMARK */
		ctrl->simulate();
	}
//#ifdef USE_STAT
	ctrl->printStats(std::cout);
	d->printStats(std::cout);
         
//#endif
}
