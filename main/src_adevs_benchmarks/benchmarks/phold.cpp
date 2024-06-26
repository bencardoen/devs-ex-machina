/*
 * phold.cpp
 *
 *  Created on: Aug 22, 2015
 *      Author: Devs Ex Machina
 */

#include <adevs.h>
#include <iostream>
#include <cstdlib>	//std::size_t
#include <cstring>
#include <sstream>
#include <string>
#include <limits>
#include <deque>
#include <random>
#include "common.h"
#include "../../main/src/tools/frandom.h"


#ifdef FPTIME
#define T_0 0.01	//timeadvance may NEVER be 0!
#define T_100 1.0
#define T_STEP 0.01
#define T_125 1.25
#define T_INF std::numeric_limits<double>::max()
#else
#define T_0 1.0	//timeadvance may NEVER be 0!
#define T_100 100.0
#define T_STEP 1.0
#define T_125 125.0
#define T_INF std::numeric_limits<double>::max()
#endif

typedef double t_eventTime;
typedef std::size_t t_payload;
typedef adevs::PortValue<t_payload, int> t_event;

struct EventPair
{
	EventPair(size_t mn, t_eventTime pt) : m_modelNumber(mn), m_procTime(pt) {};
	size_t m_modelNumber;
	t_eventTime m_procTime;
};

#ifdef FRNG
	typedef n_tools::n_frandom::t_fastrng t_randgen;
#else
	typedef std::mt19937_64 t_randgen;
#endif

std::size_t getRand(std::size_t event, t_randgen& randgen)
{
	std::uniform_int_distribution<std::size_t> dist(0, 60000);
	randgen.seed(event);
	return dist(randgen);
}

namespace n_tools {
inline std::string toString(std::size_t i)
{
#ifndef __CYGWIN__
	return std::to_string(i);
#else
	if (i == 0)
		return "0";
	std::string number = "";
	while (i != 0) {
		char c = (i % 10) + 48; // Black magic with Ascii
		i /= 10;
		number.insert(0, 1, c);
	}
	return number;
#endif
}
} /* namespace n_tools */

const int inPort = -1;

template<typename T>
constexpr T roundTo(T val, T gran)
{
	return std::round(val/gran)*gran;
}
class HeavyPHOLDProcessor: public adevs::Atomic<t_event>
{
private:
        const std::size_t m_percentageRemotes;
        const double m_percentagePriority;
	const size_t m_iter;
	const std::vector<std::size_t> m_local;
	const std::vector<std::size_t> m_remote;
	std::deque<EventPair> m_events;
	int m_messageCount;
	mutable t_randgen m_rand;

	size_t getNextDestination(size_t event) const
	{
		std::uniform_int_distribution<std::size_t> dist(0, 100);
		std::uniform_int_distribution<std::size_t> distRemote(0, m_remote.size()-1u);
		std::uniform_int_distribution<std::size_t> distLocal(0, m_local.size()-1u);
		m_rand.seed(event);
		if (dist(m_rand) > m_percentageRemotes || m_remote.empty()) {
			return m_local[distLocal(m_rand)];
		} else {
			return m_remote[distRemote(m_rand)];
		}
	}
	t_eventTime getProcTime(t_payload event) const
	{
                std::uniform_real_distribution<double> dist0(0.0, 1.0);
		std::uniform_real_distribution<t_eventTime> dist(T_100, T_125);
		m_rand.seed(event);
		double ta = roundTo(dist(m_rand), T_STEP);
                const double v = dist0(m_rand);
		if(v < m_percentagePriority){
	            return T_0;
                }
                else{
	            return ta;
                }
	}
public:
	const std::string m_name;
	HeavyPHOLDProcessor(std::string name, size_t iter, size_t modelNumber, std::vector<size_t> local,
	        std::vector<size_t> remote, size_t percentageRemotes, double percentagePriority):
		 m_percentageRemotes(percentageRemotes), m_percentagePriority(percentagePriority), m_iter(iter), m_local(local), m_remote(remote), m_messageCount(0),
		 m_name(name)
	{
                if(m_percentagePriority > 1.0 )
                        throw std::logic_error("Invalid value for priority");
		m_events.push_back(EventPair(modelNumber, getProcTime(modelNumber)));
	}

	/// Internal transition function.
	void delta_int()
	{
		if(m_events.size())
			m_events.pop_front();
	}
	/// External transition function.
	void delta_ext(double e, const adevs::Bag<t_event>& xb)
	{
		if(!m_events.empty())
			m_events[0].m_procTime -= e;
		for (auto& msg : xb) {
			++m_messageCount;
			t_payload payload = msg.value;
			m_events.push_back(EventPair(payload, getProcTime(payload)));
			volatile size_t i = 0;	//forces the compiler to include the for loop
			for (; i < m_iter;){ ++i; } 	// We just do pointless stuff for a while
		}
	}
	/// Confluent transition function.
	void delta_conf(const adevs::Bag<t_event>& xb)
	{
		if (!m_events.empty())
			m_events.pop_front();
		for (auto& msg : xb) {
			++m_messageCount;
			t_payload payload = msg.value;
			m_events.push_back(EventPair(payload, getProcTime(payload)));
			volatile size_t i = 0;	//forces the compiler to include the for loop
			for (; i < m_iter;){ ++i; } 	// We just do pointless stuff for a while
		}
	}
	/// Output function.
	void output_func(adevs::Bag<t_event>& yb)
	{
		if (!m_events.empty()) {
			EventPair& i = m_events[0];
			size_t dest = getNextDestination(i.m_modelNumber);
			size_t r = getRand(i.m_modelNumber, m_rand);
			yb.insert(t_event(dest, r));
		}
	}
	/// Time advance function.
	double ta()
	{
		if (!m_events.empty()) {
			return m_events[0].m_procTime;
		} else {
			return T_INF;
		}
	}
	/// Garbage collection
	void gc_output(adevs::Bag<t_event>&)
	{
	}

	double lookahead()
	{
		return T_0;
	}
};

class PHOLD: public adevs::Digraph<t_payload, int>
{
public:
        std::vector<HeavyPHOLDProcessor*> processors;

	PHOLD(size_t nodes, size_t atomicsPerNode, size_t iter, std::size_t percentageRemotes, double percentagePriority = 0.1)
	{
                assert(percentagePriority <= 1.0);
		std::vector<std::vector<size_t>> procs;

		for (size_t i = 0; i < nodes; ++i) {
			procs.push_back(std::vector<size_t>());
			for (size_t j = 0; j < atomicsPerNode; ++j) {
				procs[i].push_back(atomicsPerNode * i + j);
			}
		}

		size_t cntr = 0;
		for (size_t i = 0; i < nodes; ++i) {
			std::vector<size_t> allnoi;
			for (size_t k = 0; k < nodes; ++k) {
				if (i != k)
					allnoi.insert(allnoi.end(), procs[k].begin(), procs[k].end());
			}
			for (size_t num : procs[i]) {
				std::vector<size_t> inoj = procs[i];
				inoj.erase(std::remove(inoj.begin(), inoj.end(), num), inoj.end());
				HeavyPHOLDProcessor* p = new HeavyPHOLDProcessor("Processor_" + n_tools::toString(cntr),
					iter, cntr, inoj, allnoi, percentageRemotes, percentagePriority);
				processors.push_back(p);
				add((Component*)p);
				++cntr;
			}
		}

		for (size_t i = 0; i < processors.size(); ++i) {
			for (size_t j = 0; j < processors.size(); ++j) {
				if (i == j)
					continue;
				couple((Component*)(processors[i]), int(j), (Component*)(processors[j]), inPort);
			}
		}
	}


	double lookahead()
	{
		return T_STEP;
	}
};


class Listener: public adevs::EventListener<t_event>
{
public:

	virtual void outputEvent(adevs::Event<t_event,double> x, double t){
		std::cout << "an output event at time " << t;
		HeavyPHOLDProcessor* proc = dynamic_cast<HeavyPHOLDProcessor*>(x.model);
		if(proc != nullptr)
			std::cout << "  (proc " << proc->m_name << ")";
		std::cout << '\n';

	}
	virtual void stateChange(adevs::Atomic<t_event>* model, double t){
		std::cout << "an event at time " << t << "!";
		HeavyPHOLDProcessor* proc = dynamic_cast<HeavyPHOLDProcessor*>(model);
		if(proc != nullptr)
			std::cout << "  (proc " << proc->m_name << ")";
		std::cout << '\n';
	}

	virtual ~Listener(){}
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


const char helpstr[] = " [-h] [-t ENDTIME] [-n NODES] [-s SUBNODES] [-r REMOTES] [-p PRIORITY] [-i ITER] [-c COREAMT] [classic|cpdevs]\n"
	"options:\n"
	"  -h             show help and exit\n"
	"  -t ENDTIME     set the endtime of the simulation\n"
	"  -n NODES       number of phold nodes\n"
	"  -s SUBNODES    number of subnodes per phold node\n"
	"  -r REMOTES     percentage of remote connections\n"
    "  -p PRIORITY    chance of a priority event. Must be within the range [0.0, 1.0]\n"
	"  -i ITER        amount of useless work to simulate complex calculations\n"
	"  -c COREAMT     amount of simulation cores, ignored in classic mode\n"
	"  classic        Run single core simulation.\n"
	"  cpdevs         Run conservative parallel simulation.\n"
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
        const char optPriority = 'p';
	const char optCores = 'c';
	char** argvc = argv+1;

	double eTime = 50;
	std::size_t nodes = 1;
	std::size_t subnodes = 10;
	std::size_t remotes = 10;
	double priority = 0.1;
	std::size_t iter = 0;

	bool hasError = false;
	bool isClassic = true;
	std::size_t coreAmt = 4;

	for(int i = 1; i < argc; ++argvc, ++i){
		char c = getOpt(*argvc);
		if(!c){
			if(!strcmp(*argvc, "classic")){
				isClassic = true;
				continue;
			} else if(!strcmp(*argvc, "cpdevs")){
				isClassic = false;
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
				eTime = toData<double>(std::string(*(++argvc)));
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
				subnodes = toData<std::size_t>(std::string(*(++argvc)));
			} else {
				std::cout << "Missing argument for option -" << optDepth << '\n';
			}
			break;
        case optRemote:
            ++i;
            if(i < argc){
                remotes = toData<std::size_t>(std::string(*(++argvc)));
            } else {
                std::cout << "Missing argument for option -" << optRemote << '\n';
            }
            break;
        case optPriority:
            ++i;
            if(i < argc){
                priority = toData<double>(std::string(*(++argvc)));
            } else {
                std::cout << "Missing argument for option -" << optPriority << '\n';
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

	adevs::Devs<t_event>* model = new PHOLD(nodes, subnodes, iter, remotes, priority);
#ifdef USE_STAT
    #define USE_LISTENER
    adevs::EventListener<t_event>* listener = new OutputCounter<t_event>();
#else
#ifndef BENCHMARK
    #define USE_LISTENER
    adevs::EventListener<t_event>* listener = new Listener();
#endif //#ifndef BENCHMARK
#endif //#ifdef USE_STAT
	if(isClassic){
		adevs::Simulator<t_event> sim(model);
#ifdef USE_LISTENER
		sim.addEventListener(listener);
#endif
		sim.execUntil(eTime);
	} else {
		omp_set_num_threads(coreAmt);	//must manually set amount of OpenMP threads
		std::size_t i = 0;
		std::size_t nodes_per_core = std::ceil(((PHOLD*)model)->processors.size() / (double)coreAmt);
		for(HeavyPHOLDProcessor* ptr: ((PHOLD*)model)->processors){
                        size_t coreid = i / nodes_per_core;
                        if (coreid >= coreAmt) {     // overflow into the last core.
                                coreid = coreAmt - 1;
                        }
//                        std::cout << "assigned " << ptr->m_name << " to core " << coreid << '\n';
                        ptr->setProc(coreid);
	                    ++i;
		}
		adevs::ParSimulator<t_event> sim(model);
#ifdef USE_LISTENER
		sim.addEventListener(listener);
#endif
		sim.execUntil(eTime);
	}
#ifdef USE_LISTENER
	delete listener;
#endif
	delete model;
}
