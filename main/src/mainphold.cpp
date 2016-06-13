/*
 * This file is part of the DEVS Ex Machina project.
 * Copyright 2014 - 2015 University of Antwerp
 * https://www.uantwerpen.be/en/
 * Licensed under the EUPL V.1.1
 * A full copy of the license is in COPYING.txt, or can be found at
 * https://joinup.ec.europa.eu/community/eupl/og_page/eupl
 *      Author: Ben Cardoen, Stijn Manhaeve, Tim Tuijn, Matthijs Van Os
 */

#include <performance/phold/phold.h>
#include "control/controllerconfig.h"
#include "tools/coutredirect.h"
#include "tools/stringtools.h"

using namespace n_tools;

LOG_INIT("phold.log")

const char helpstr[] = " [-h] [-t ENDTIME] [-n NODES] [-s SUBNODES] [-r REMOTES] [-p PRIORITY] [-i ITER] [-S seed] [-c COREAMT] [classic|cpdevs|opdevs|pdevs]\n"
	"options:\n"
	"  -h             show help and exit\n"
	"  -t ENDTIME     set the endtime of the simulation\n"
	"  -n NODES       number of phold nodes\n"
	"  -s SUBNODES    number of subnodes per phold node\n"
	"  -r REMOTES     percentage of remote connections\n"
    "  -p PRIORITY    chance of a priority event. Must be within the range [0.0, 1.0]\n"
	"  -i ITER        amount of useless work to simulate complex calculations\n"
	"  -c COREAMT     amount of simulation cores, ignored in classic mode. This should be exactly equal to the n argument.\n"
    "  -S seed        Initial seed with which all random number generators are seeded.\n"
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
	const char optDepth = 's';
	const char optHelp = 'h';
	const char optIter = 'i';
    const char optRemote = 'r';
    const char optPriority = 'p';
	const char optCores = 'c';
    const char optSeed = 'S';
	char** argvc = argv+1;

#ifdef FPTIME
	n_network::t_timestamp::t_time eTime = 50.0;
#else
	n_network::t_timestamp::t_time eTime = 50;
#endif
	n_benchmarks_phold::PHOLDConfig conf;
	conf.nodes = 4;  // Can't pick a default of c=4 with n=1.
	conf.atomicsPerNode = 10;
	conf.percentageRemotes = 10;
	conf.percentagePriority = 0.1;
	conf.iter = 0;
    conf.initialSeed = 1;

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
				conf.nodes = toData<std::size_t>(std::string(*(++argvc)));
			} else {
				std::cout << "Missing argument for option -" << optWidth << '\n';
			}
			break;
		case optDepth:
			++i;
			if(i < argc){
				conf.atomicsPerNode = toData<std::size_t>(std::string(*(++argvc)));
                                if(conf.atomicsPerNode < 2){
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
                conf.percentageRemotes = toData<std::size_t>(std::string(*(++argvc)));
            } else {
                std::cout << "Missing argument for option -" << optRemote << '\n';
            }
            break;
        case optPriority:
            ++i;
            if(i < argc){
                conf.percentagePriority = toData<double>(std::string(*(++argvc)));
            } else {
                std::cout << "Missing argument for option -" << optPriority << '\n';
            }
            break;
		case optIter:
			++i;
			if(i < argc){
				conf.iter = toData<std::size_t>(std::string(*(++argvc)));
			} else {
				std::cout << "Missing argument for option -" << optIter << '\n';
			}
			break;
        case optSeed:
            ++i;
            if(i < argc){
                conf.initialSeed = toData<std::size_t>(std::string(*(++argvc)));
                if(conf.initialSeed == 0){
                    std::cout << "Invalid argument for option -" << optSeed << "\n  note: seed '0' is not allowed.\n";
                    hasError = true;
                }
            } else {
                std::cout << "Missing argument for option -" << optSeed << '\n';
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
        if(conf.nodes != coreAmt && simType!=n_control::SimType::CLASSIC){
                std::cerr << conf.nodes << std::endl;
                std::cerr << coreAmt << std::endl;
                throw std::logic_error("N should match C");
        }
	n_control::ControllerConfig cconf;
	cconf.m_name = "PHOLD";
	cconf.m_simType = simType;
	cconf.m_coreAmount = coreAmt;
	cconf.m_saveInterval = 5;
	cconf.m_allocator = n_tools::createObject<n_benchmarks_phold::PHoldAlloc>();

	auto ctrl = cconf.createController();
	t_timestamp endTime(eTime, 0);
	ctrl->setTerminationTime(endTime);
	t_coupledmodelptr d = n_tools::createObject<n_benchmarks_phold::PHOLD>(conf);
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
        //note that these can create huge amounts of data
        std::ofstream outstats("stats.txt");
        //ctrl->printStats(outstats);
        d->printStats(outstats);
#endif
}
