/*
 * This file is part of the DEVS Ex Machina project.
 * Copyright 2014 - 2015 University of Antwerp
 * https://www.uantwerpen.be/en/
 * Licensed under the EUPL V.1.1
 * A full copy of the license is in COPYING.txt, or can be found at
 * https://joinup.ec.europa.eu/community/eupl/og_page/eupl
 *      Author: Ben Cardoen, Stijn Manhaeve
 */

#include "control/controllerconfig.h"
#include "tools/coutredirect.h"
#include "performance/queuenetwork/queuenetwork.h"
#include "tools/stringtools.h"
#include "control/allocator.h"
#include "tools/statistic.h"

using namespace n_tools;

LOG_INIT("queuenetwork.log")


/**
 * cmd args:
 * [-h] [-t ENDTIME] [-w WIDTH] [-p PRIORITY] [-f] [-c COREAMT] [classic|cpdevs|opdevs|pdevs]
 * 	-h: show help and exit
 * 	-t ENDTIME set the endtime of the simulation
 * 	-w WIDTH the amount of generators in the server queue model
 * 	-p PRIORITY the chance of a prioritized message being generated
 * 	-f use the feedback model. This model sends the generated messages from the splitter back to the generators
 * 	-c COREAMT amount of simulation cores, ignored in classic mode
 * 	classic run single core simulation
 * 	cpdevs run conservative parallel simulation
 * 	opdevs|pdevs run optimistic parallel simulation
 * The last value entered for an option will overwrite any previous values for that option.
 */
const char helpstr[] = " [-h] [-t ENDTIME] [-w WIDTH] [-p PRIORITY] [-f] [-c COREAMT] [classic|cpdevs|opdevs|pdevs]\n"
	"options:\n"
	"  -h           show help and exit\n"
	"  -t ENDTIME   set the endtime of the simulation\n"
	"  -w WIDTH     the amount of generators in the server queue model\n"
	"  -p PRIORITY  the chance of a prioritized message being generated\n"
	"  -f           use the feedback model. This model sends the generated messages from the splitter back to the generators\n"
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
	const char optPriority = 'p';
	const char optHelp = 'h';
	const char optFeedback = 'f';
	const char optCores = 'c';
	char** argvc = argv+1;

#ifdef FPTIME
	n_network::t_timestamp::t_time eTime = 5000.0;
#else
	n_network::t_timestamp::t_time eTime = 5000;
#endif
	std::size_t width = 2;
	std::size_t priority = 10;
	bool feedback = false;

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
				hasError = true;
			}
			break;
		case optETime:
			++i;
			if(i < argc){
				eTime = toData<n_network::t_timestamp::t_time>(std::string(*(++argvc)));
			} else {
				std::cout << "Missing argument for option -" << optETime << '\n';
				hasError = true;
			}
			break;
		case optWidth:
			++i;
			if(i < argc){
				width = toData<std::size_t>(std::string(*(++argvc)));
			} else {
				std::cout << "Missing argument for option -" << optWidth << '\n';
				hasError = true;
			}
			break;
		case optPriority:
			++i;
			if(i < argc){
				priority = toData<std::size_t>(std::string(*(++argvc)));
				if(priority > 100){
					std::cout << "The priority must be in the range [0, 100]";
					hasError = true;
				}
			} else {
				std::cout << "Missing argument for option -" << optPriority << '\n';
				hasError = true;
			}
			break;
		case optFeedback:
			feedback = true;
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
	conf.m_saveInterval = 250;     
	conf.m_allocator = n_tools::createObject<QueueAlloc>();

	auto ctrl = conf.createController();
	t_timestamp endTime(eTime, 0);
	ctrl->setTerminationTime(endTime);

	t_coupledmodelptr d = nullptr;
	if(!feedback)
		d = std::static_pointer_cast<n_model::CoupledModel>(n_tools::createObject<n_queuenetwork::SingleServerNetwork>(width, width, priority, 100));
	else
		d = std::static_pointer_cast<n_model::CoupledModel>(n_tools::createObject<n_queuenetwork::FeedbackServerNetwork>(width, priority, 100));
	ctrl->addModel(d);
	{
#ifndef BENCHMARK
		std::ofstream filestream("./queuenetwork.txt");
		n_tools::CoutRedirect myRedirect(filestream);
#endif /* BENCHMARK */
		ctrl->simulate();
	}
#ifdef USE_STAT
    //note that these can create huge amounts of data
    std::ofstream outstats("stats.txt");
    //ctrl->printStats(outstats);
    d->printStats(outstats);
#endif
}
