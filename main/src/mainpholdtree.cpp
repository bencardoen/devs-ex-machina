/*
 * This file is part of the DEVS Ex Machina project.
 * Copyright 2014 - 2015 University of Antwerp
 * https://www.uantwerpen.be/en/
 * Licensed under the EUPL V.1.1
 * A full copy of the license is in COPYING.txt, or can be found at
 * https://joinup.ec.europa.eu/community/eupl/og_page/eupl
 *      Author: Ben Cardoen, Stijn Manhaeve, Tim Tuijn, Matthijs Van Os
 */

#include <performance/pholdtree/pholdtree.h>
#include "control/controllerconfig.h"
#include "tools/coutredirect.h"
#include "tools/stringtools.h"

using namespace n_tools;

LOG_INIT("pholdtree.log")

const char helpstr[] = " [-h] [-t ENDTIME] [-n NODES] [-d depth] [-p PRIORITY] [-i ITER] [-c COREAMT] [classic|cpdevs|opdevs|pdevs]\n"
	"options:\n"
	"  -h             show help and exit\n"
	"  -t ENDTIME     set the endtime of the simulation\n"
	"  -n NODES       number of pholdtree nodes per tree node\n"
	"  -d DEPTH       depth of the pholdtree\n"
    "  -p PRIORITY    chance of a priority event. Must be within the range [0.0, 1.0]\n"
	"  -c COREAMT     amount of simulation cores, ignored in classic mode. This should be exactly equal to the n argument!!!\n"
	"  classic        Run single core simulation.\n"
	"  cpdevs         Run conservative parallel simulation.\n"
	"  opdevs|pdevs   Run optimistic parallel simulation.\n"
	"note:\n"
	"  If the same option is set multiple times, only the last value is taken.\n";
int main(int argc, char** argv)
{
	LOG_ARGV(argc, argv);
	const char optETime = 't';
	const char optWidth = 'n';
	const char optDepth = 'd';
	const char optHelp = 'h';
    const char optPriority = 'p';
	const char optCores = 'c';
	char** argvc = argv+1;

#ifdef FPTIME
	n_network::t_timestamp::t_time eTime = 50.0;
#else
	n_network::t_timestamp::t_time eTime = 50;
#endif
	n_benchmarks_pholdtree::PHOLDTreeConfig config;
	config.numChildren = 4;
    config.percentagePriority = 0.1;
	std::size_t depth = 3;

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
				config.numChildren = toData<std::size_t>(std::string(*(++argvc)));
			} else {
				std::cout << "Missing argument for option -" << optWidth << '\n';
			}
			if(config.numChildren == 0) {
                std::cout << "Option -" << optWidth << " can't have a value of 0.\n";
			}
			break;
		case optDepth:
			++i;
			if(i < argc){
				depth = toData<std::size_t>(std::string(*(++argvc)));
			} else {
				std::cout << "Missing argument for option -" << optDepth << '\n';
			}
			break;
        case optPriority:
            ++i;
            if(i < argc){
                config.percentagePriority = toData<double>(std::string(*(++argvc)));
            } else {
                std::cout << "Missing argument for option -" << optPriority << '\n';
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
	conf.m_name = "PHOLDTree";
	conf.m_simType = simType;
	conf.m_coreAmount = coreAmt;
	conf.m_saveInterval = 5;
	conf.m_allocator = n_tools::createObject<n_benchmarks_pholdtree::PHoldTreeAlloc>();

	auto ctrl = conf.createController();
	t_timestamp endTime(eTime, 0);
	ctrl->setTerminationTime(endTime);
	t_coupledmodelptr d = n_tools::createObject<n_benchmarks_pholdtree::PHOLDTree>(config, depth);
	ctrl->addModel(d);
	{
#ifndef BENCHMARK
		std::ofstream filestream("./pholdtree.txt");
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
