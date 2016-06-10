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
#define T_0 0.0
#define T_1 0.01
#define T_50 0.5
#define T_75 0.75
#define T_100 1.0
#define T_125 1.25
#define T_STEP 0.01
double T_INF = std::numeric_limits<double>::max();
#else
#define T_0 0.0
#define T_1 1.0
#define T_50 50.0
#define T_75 75.0
#define T_100 100.0
#define T_125 125.0
#define T_STEP 1.0
double T_INF = std::numeric_limits<double>::max();
#endif

typedef double t_eventTime;
typedef std::size_t t_payload;
typedef adevs::PortValue<t_payload, int> t_event;

#ifdef FRNG
	typedef n_tools::n_frandom::t_fastrng t_randgen;
#else
	typedef std::mt19937_64 t_randgen;
#endif

typedef boost::random::taus88 t_seedrandgen;    //this random generator will be used to generate the initial seeds
//it MUST be diferent from the regular t_randgen
static_assert(!std::is_same<t_randgen, t_seedrandgen>::value, "The rng for the seed can't be the same random number generator as the one for he random events.");


constexpr int outPort = 0;
constexpr int inPort = 1;

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


template<typename T>
constexpr T roundTo(T val, T gran)
{
	return std::round(val/gran)*gran;
}

class Generator: public adevs::Atomic<t_event>
{
private:
	const bool m_randomta;
	mutable t_randgen m_rand;
	double m_count;

	void adjustCounter()
	{

		if(!m_randomta){
			m_count = T_100;
			return;
		}
#ifdef FPTIME
        std::uniform_real_distribution<double> dist(T_1, T_100);
        m_count = roundTo(dist(m_rand), T_STEP);
#else
        std::uniform_int_distribution<std::size_t> dist(T_1, T_100);
        m_count = double(dist(m_rand));
#endif
	}
public:
	std::string m_name;
	Generator(const std::string& name, std::size_t seed, bool randTa):
		 m_randomta(randTa), m_count(0), m_name(name)
	{
	    m_rand.seed(seed);
		adjustCounter();
	}

	/// Internal transition function.
	void delta_int()
	{
		adjustCounter();
	}
	/// External transition function.
	void delta_ext(double e, const adevs::Bag<t_event>&)
	{
		m_count -= e;
	}
	/// Confluent transition function.
	void delta_conf(const adevs::Bag<t_event>&)
	{
		adjustCounter();
	}
	/// Output function.
	void output_func(adevs::Bag<t_event>& yb)
	{
		yb.insert(t_event(outPort, 42));
	}
	/// Time advance function.
	double ta()
	{
		return m_count;
	}
	/// Garbage collection
	void gc_output(adevs::Bag<t_event>&)
	{
	}

	double lookahead()
	{
		return m_randomta? T_STEP : T_100;
	}
};

class HighInterconnect: public adevs::Digraph<t_payload, int>
{
public:
        std::vector<Generator*> ptrs;
	HighInterconnect(std::size_t width, bool randomta, std::size_t initialSeed)
	{
	    t_seedrandgen getSeed;
	    getSeed.seed(initialSeed);
		for(std::size_t i = 0; i < width; ++i){
			Generator* pt = new Generator(std::string("Generator") + n_tools::toString(i), getSeed(), randomta);
                        ptrs.push_back(pt);
                        add(pt);
		}
		for(std::size_t i = 0; i < width; ++i){
			for(std::size_t j = 0; j < width; ++j){
				if(i != j){
					couple(ptrs[i], outPort, ptrs[j], inPort);
                                }
			}
		}
	}


	double lookahead()
	{
		return T_0;
	}
};

struct IconnectName {
        static std::string eval(adevs::Devs<t_event, double>* model) {
            Generator* proc = dynamic_cast<Generator*>(model);
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


const char helpstr[] = " [-h] [-t ENDTIME] [-w WIDTH] [-r] [-S seed] [-c COREAMT] [classic|cpdevs]\n"
	"options:\n"
	"  -h           show help and exit\n"
	"  -t ENDTIME   set the endtime of the simulation\n"
	"  -w WIDTH     the width of the high interconnect model\n"
	"  -r           use randomized processing time\n"
    "  -S seed      Initial seed with which all random number generators are seeded.\n"
	"  -c COREAMT   amount of simulation cores, ignored in classic mode. Must not be 0.\n"
	"  classic      Run single core simulation.\n"
	"  cpdevs       Run conservative parallel simulation.\n"
	"note:\n"
	"  If the same option is set multiple times, only the last value is taken.\n";

int main(int argc, char** argv)
{
	const char optETime = 't';
	const char optWidth = 'w';
	const char optHelp = 'h';
	const char optRand = 'r';
	const char optCores = 'c';
    const char optSeed = 'S';
	char** argvc = argv+1;

	double eTime = 50;
	std::size_t width = 2;
	bool randTa = false;
    std::size_t initialSeed = 42;


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
				width = toData<std::size_t>(std::string(*(++argvc)));
			} else {
				std::cout << "Missing argument for option -" << optWidth << '\n';
			}
			break;
		case optRand:
			randTa = true;
			break;
        case optSeed:
            ++i;
            if(i < argc){
                initialSeed = toData<std::size_t>(std::string(*(++argvc)));
                if(initialSeed == 0){
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

	adevs::Devs<t_event>* model = new HighInterconnect(width, randTa, initialSeed);
#ifdef USE_STAT
#undef USE_STAT
#endif
#ifdef USE_STAT
    #define USE_LISTENER
    adevs::EventListener<t_event>* listener = new OutputCounter<t_event>();
#else
#ifndef BENCHMARK
    #define USE_LISTENER
    adevs::EventListener<t_event>* listener = new Listener<t_event, IconnectName>();
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
		size_t i = 0;
		for(Generator* ptr: ((HighInterconnect*)model)->ptrs){
		                        i = (i+1)%coreAmt;
		                        ptr->setProc(i);
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
