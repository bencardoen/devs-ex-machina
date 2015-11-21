/*
 * maindevstone.cpp
 *
 *      Author: Devs Ex Machina
 */

#include "control/controllerconfig.h"
#include "tools/coutredirect.h"
#include "performance/prioritymessage/prioritymessage.h"
#include "tools/stringtools.h"

#include "tools/statistic.h"

LOG_INIT("prioritymsg.log")

using namespace n_tools;

/**
 * cmd args:
 * [-h] [-t ENDTIME] [-n NODES] [-m MESSAGES] [-r] [-p PRIORITY] [classic|cpdevs|opdevs|pdevs]
 * 	-h: show help and exit
 * 	-t ENDTIME set the endtime of the simulation
 * 	-n NODES the number of receiving nodes
 * 	-m MESSAGES the number of generated messages each time
 * 	-p PRIORITY chance for generating a priority message
 * 	classic run single core simulation
 * 	cpdevs run conservative parallel simulation
 * 	opdevs|pdevs run optimistic parallel simulation
 * The last value entered for an option will overwrite any previous values for that option.
 */
const char helpstr[] = " [-h] [-t ENDTIME] [-n NODES] [-m MESSAGES] [-r] [-p PRIORITY] [classic|cpdevs|opdevs|pdevs]\n"
	"options:\n"
	"  -h           show help and exit\n"
	"  -t ENDTIME   set the endtime of the simulation\n"
	"  -n NODES     the number of receiving nodes\n"
	"  -m MESSAGES  the number of generated messages each time\n"
	"  -p PRIORITY  chance for generating a priority message\n"
	"  classic      Run single core simulation.\n"
	"  cpdevs       Run conservative parallel simulation.\n"
	"  opdevs|pdevs Run optimistic parallel simulation.\n"
	"note:\n"
	"  If the same option is set multiple times, only the last value is taken.\n";

int main(int argc, char** argv)
{
	LOG_ARGV(argc, argv);
	// default values:
	const char optETime = 't';
	const char optNodes = 'n';
	const char optMessages = 'm';
	const char optHelp = 'h';
	const char optChance = 'p';
	char** argvc = argv+1;

#ifdef FPTIME
	n_network::t_timestamp::t_time eTime = 50.0;
#else
	n_network::t_timestamp::t_time eTime = 50;
#endif
	std::size_t numRec = 2;
	std::size_t numMsg = 3;
	std::size_t chance = 10;

	bool hasError = false;
	n_control::SimType simType = n_control::SimType::CLASSIC;
	std::size_t coreAmt = 2;

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
		case optETime:
			++i;
			if(i < argc){
				eTime = toData<n_network::t_timestamp::t_time>(std::string(*(++argvc)));
			} else {
				std::cout << "Missing argument for option -" << optETime << '\n';
			}
			break;
		case optMessages:
			++i;
			if(i < argc){
				numMsg = toData<std::size_t>(std::string(*(++argvc)));
			} else {
				std::cout << "Missing argument for option -" << optMessages << '\n';
			}
			break;
		case optNodes:
			++i;
			if(i < argc){
				numRec = toData<std::size_t>(std::string(*(++argvc)));
			} else {
				std::cout << "Missing argument for option -" << optNodes << '\n';
			}
			break;
        case optChance:
            ++i;
            if(i < argc){
                chance = toData<std::size_t>(std::string(*(++argvc)));
            } else {
                std::cout << "Missing argument for option -" << optChance << '\n';
            }
            if(chance > 100) {
                    std::cout << "Illegal chance of creating prioritized messages: " << chance << '\n';
                    hasError = true;
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
	conf.m_name = "Priority";
	conf.m_simType = simType;
	conf.m_coreAmount = coreAmt;
	conf.m_saveInterval = 250;     

	auto ctrl = conf.createController();
	t_timestamp endTime(eTime, 0);
	ctrl->setTerminationTime(endTime);

	t_coupledmodelptr d = n_tools::createObject< n_priorityMsg::PriorityMessage>(numMsg, numRec, chance);
	ctrl->addModel(d);
	{
#ifndef BENCHMARK
		std::ofstream filestream("./prioritymsg.txt");
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
