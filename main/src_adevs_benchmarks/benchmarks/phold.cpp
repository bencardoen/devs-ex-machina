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
#include <boost/random.hpp>
#include "../../main/src/tools/frandom.h"

#ifdef FPTIME
#define T_0 0.01    //timeadvance may NEVER be 0!
#define T_100 1.0
#define T_STEP 0.01
#define T_125 1.25
double T_INF = std::numeric_limits<double>::max();
#else
#define T_0 1.0 //timeadvance may NEVER be 0!
#define T_100 100.0
#define T_STEP 1.0
#define T_125 125.0
double T_INF = std::numeric_limits<double>::max();
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
typedef boost::random::taus88 t_seedrandgen;    //this random generator will be used to generate the initial seeds
                                            //it MUST be diferent from the regular t_randgen
static_assert(!std::is_same<t_randgen, t_seedrandgen>::value, "The rng for the seed can't be the same random number generator as the one for he random events.");


std::size_t getRand(std::size_t, t_randgen& randgen)
{
	std::uniform_int_distribution<std::size_t> dist(0, 60000);
//	randgen.seed(event);
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
    std::size_t m_destination;  //the next destination
    std::size_t m_nextMessage;  //the next message

	size_t getNextDestination(size_t) const
	{
		std::uniform_int_distribution<std::size_t> dist(0, 100);
		std::uniform_int_distribution<std::size_t> distRemote(0, m_remote.size()-1u);
		std::uniform_int_distribution<std::size_t> distLocal(0, m_local.size()-1u);
//		m_rand.seed(event);
		if (dist(m_rand) > m_percentageRemotes || m_remote.empty()) {
			return m_local[distLocal(m_rand)];
		} else {
			return m_remote[distRemote(m_rand)];
		}
	}
	t_eventTime getProcTime(t_payload) const
	{
        std::uniform_real_distribution<double> dist0(0.0, 1.0);
#ifdef FPTIME
		std::uniform_real_distribution<t_eventTime> dist(T_100, T_125);
        double ta = roundTo(dist(m_rand), T_STEP);
#else
		std::uniform_int_distribution<std::size_t> dist(T_100, T_125);
        double ta = double(dist(m_rand));
#endif
//		m_rand.seed(event);
        const double v = dist0(m_rand);
		if(v < m_percentagePriority){
            return T_0;
        } else {
            return ta;
        }
	}
public:
	const std::string m_name;
	HeavyPHOLDProcessor(std::string name, size_t iter, size_t modelNumber, std::vector<size_t> local,
	        std::vector<size_t> remote, size_t percentageRemotes, double percentagePriority, std::size_t startSeed):
		 m_percentageRemotes(percentageRemotes), m_percentagePriority(percentagePriority), m_iter(iter), m_local(local), m_remote(remote), m_messageCount(0),
		 m_destination(0), m_nextMessage(0),
		 m_name(name)
	{
        if(m_percentagePriority > 1.0 )
                throw std::logic_error("Invalid value for priority");
        m_rand.seed(startSeed);
		m_events.push_back(EventPair(modelNumber, getProcTime(modelNumber)));
        EventPair& i = m_events[0];
        m_destination = getNextDestination(i.m_modelNumber);
        m_nextMessage = getRand(i.m_modelNumber, m_rand);
        std::cerr << "%remotes:" << m_percentageRemotes << '\n';
	}

	/// Internal transition function.
	void delta_int()
	{
		if(m_events.size())
			m_events.pop_front();

        if(!m_events.empty()) {
            EventPair& i = m_events[0];
            m_destination = getNextDestination(i.m_modelNumber);
            m_nextMessage = getRand(i.m_modelNumber, m_rand);
        }
	}
	/// External transition function.
	void delta_ext(double e, const adevs::Bag<t_event>& xb)
	{
		if(!m_events.empty())
			m_events[0].m_procTime -= e;
		bool wasEmpty = m_events.empty();
		for (auto& msg : xb) {
			++m_messageCount;
			t_payload payload = msg.value;
			m_events.push_back(EventPair(payload, getProcTime(payload)));
			volatile size_t i = 0;	//forces the compiler to include the for loop
			for (; i < m_iter;){ ++i; } 	// We just do pointless stuff for a while
		}
		if(wasEmpty && !m_events.empty()) {
            EventPair& i = m_events[0];
            m_destination = getNextDestination(i.m_modelNumber);
            m_nextMessage = getRand(i.m_modelNumber, m_rand);
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
        if(!m_events.empty()) {
            EventPair& i = m_events[0];
            m_destination = getNextDestination(i.m_modelNumber);
            m_nextMessage = getRand(i.m_modelNumber, m_rand);
        }
	}
	/// Output function.
	void output_func(adevs::Bag<t_event>& yb)
	{
		if (!m_events.empty()) {
			size_t dest = m_destination;
			size_t r = m_nextMessage;
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

struct PHOLDConfig
{
        std::size_t nodes;
        std::size_t atomicsPerNode;
        std::size_t iter;
        double percentageRemotes;
        double percentagePriority;

        std::size_t initialSeed;         //the initial seed from which the model rng's are initialized. The default is 42, because some rng can't handle seed 0
        t_seedrandgen getSeed;      //the random number generator for getting the new seeds.

        PHOLDConfig(): nodes(0u), atomicsPerNode(0u), iter(0u),
                        percentageRemotes(0.1), percentagePriority(0.1),
                        initialSeed(42)
        {}

};

class PHOLD: public adevs::Digraph<t_payload, int>
{
public:
        std::vector<HeavyPHOLDProcessor*> processors;

	PHOLD(PHOLDConfig config)
	{
        assert(config.percentagePriority <= 1.0);
		std::vector<std::vector<size_t>> procs;
		config.getSeed.seed(config.initialSeed);

		for (size_t i = 0; i < config.nodes; ++i) {
			procs.push_back(std::vector<size_t>());
			for (size_t j = 0; j < config.atomicsPerNode; ++j) {
				procs[i].push_back(config.atomicsPerNode * i + j);
			}
		}

		size_t cntr = 0;
		for (size_t i = 0; i < config.nodes; ++i) {
			std::vector<size_t> allnoi;
			for (size_t k = 0; k < config.nodes; ++k) {
				if (i != k)
					allnoi.insert(allnoi.end(), procs[k].begin(), procs[k].end());
			}
			for (size_t num : procs[i]) {
				std::vector<size_t> inoj = procs[i];
				inoj.erase(std::remove(inoj.begin(), inoj.end(), num), inoj.end());
				HeavyPHOLDProcessor* p = new HeavyPHOLDProcessor("Processor_" + n_tools::toString(cntr),
				                                                 config.iter, cntr, inoj, allnoi, config.percentageRemotes, config.percentagePriority, config.getSeed());
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

struct PHOLDName {
        static std::string eval(adevs::Devs<t_event, double>* model) {
            HeavyPHOLDProcessor* proc = dynamic_cast<HeavyPHOLDProcessor*>(model);
            if(proc != nullptr) return proc->m_name;
            return "???";
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


const char helpstr[] = " [-h] [-t ENDTIME] [-n NODES] [-s SUBNODES] [-r REMOTES] [-p PRIORITY] [-i ITER] [-S seed] [-c COREAMT] [classic|cpdevs]\n"
	"options:\n"
	"  -h             show help and exit\n"
	"  -t ENDTIME     set the endtime of the simulation\n"
	"  -n NODES       number of phold nodes\n"
	"  -s SUBNODES    number of subnodes per phold node\n"
	"  -r REMOTES     percentage of remote connections\n"
    "  -p PRIORITY    chance of a priority event. Must be within the range [0.0, 1.0]\n"
	"  -i ITER        amount of useless work to simulate complex calculations\n"
    "  -S seed        Initial seed with which all random number generators are seeded.\n"
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
    const char optSeed = 'S';
	char** argvc = argv+1;

	double eTime = 50;
    PHOLDConfig phconf;
    phconf.nodes = 4;
    phconf.atomicsPerNode = 10;
    phconf.percentageRemotes = 10;
    phconf.percentagePriority = 0.1;
    phconf.iter = 0;
    phconf.initialSeed = 1;

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
			    phconf.nodes = toData<std::size_t>(std::string(*(++argvc)));
			} else {
				std::cout << "Missing argument for option -" << optWidth << '\n';
			}
			break;
		case optDepth:
			++i;
			if(i < argc){
			    phconf.atomicsPerNode= toData<std::size_t>(std::string(*(++argvc)));
			} else {
				std::cout << "Missing argument for option -" << optDepth << '\n';
			}
			break;
        case optRemote:
            ++i;
            if(i < argc){
                phconf.percentageRemotes= toData<std::size_t>(std::string(*(++argvc)));
            } else {
                std::cout << "Missing argument for option -" << optRemote << '\n';
            }
            break;
        case optPriority:
            ++i;
            if(i < argc){
                phconf.percentagePriority = toData<double>(std::string(*(++argvc)));
            } else {
                std::cout << "Missing argument for option -" << optPriority << '\n';
            }
            break;
		case optIter:
			++i;
			if(i < argc){
			    phconf.iter = toData<std::size_t>(std::string(*(++argvc)));
			} else {
				std::cout << "Missing argument for option -" << optIter << '\n';
			}
			break;
        case optSeed:
            ++i;
            if(i < argc){
                phconf.initialSeed = toData<std::size_t>(std::string(*(++argvc)));
                if(phconf.initialSeed == 0){
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

	adevs::Devs<t_event>* model = new PHOLD(phconf);
#ifdef USE_STAT
#undef USE_STAT
#endif
#ifdef USE_STAT
    #define USE_LISTENER
    adevs::EventListener<t_event>* listener = new OutputCounter<t_event>();
#else
#ifndef BENCHMARK
    #define USE_LISTENER
    adevs::EventListener<t_event>* listener = new Listener<t_event, PHOLDName>();
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
		std::cout << "setting up listener\n";
#endif
		sim.execUntil(eTime);
	}
#ifdef USE_LISTENER
	delete listener;
#endif
	delete model;
}
