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
#include "../../main/src/tools/frandom.h"


#ifdef FPTIME
#define T_0 0.0
#define T_1 0.01
#define T_50 0.5
#define T_75 0.75
#define T_100 1.0
#define T_125 1.25
#define T_STEP 0.01
#else
#define T_0 0.0
#define T_1 1.0
#define T_50 50.0
#define T_75 75.0
#define T_100 100.0
#define T_125 125.0
#define T_STEP 1.0
#endif

typedef double t_eventTime;
typedef std::size_t t_payload;
typedef adevs::PortValue<t_payload, int> t_event;

#ifdef FRNG
	typedef n_tools::n_frandom::t_fastrng t_randgen;
#else
	typedef std::mt19937_64 t_randgen;
#endif

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
	std::size_t m_seed;

	void adjustCounter(std::size_t seed)
	{

		if(!m_randomta){
			m_count = T_100;
			return;
		}
		std::uniform_real_distribution<double> dist(T_1, T_100);
		std::uniform_int_distribution<std::size_t> dist2(0, std::numeric_limits<std::size_t>::max());
		m_rand.seed(seed);
		m_count = dist(m_rand);
		m_count = roundTo(m_count, T_STEP);
		m_seed = dist2(m_rand);
	}
public:
	std::string m_name;
	Generator(const std::string& name, std::size_t seed, bool randTa):
		 m_randomta(randTa), m_count(0), m_seed(0), m_name(name)
	{
		adjustCounter(seed);
	}

	/// Internal transition function.
	void delta_int()
	{
		adjustCounter(m_seed);
	}
	/// External transition function.
	void delta_ext(double e, const adevs::Bag<t_event>&)
	{
		m_count -= e;
	}
	/// Confluent transition function.
	void delta_conf(const adevs::Bag<t_event>&)
	{
		adjustCounter(m_seed);
	}
	/// Output function.
	void output_func(adevs::Bag<t_event>& yb)
	{
		yb.insert(t_event(outPort, m_seed));
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
	HighInterconnect(std::size_t width, bool randomta)
	{
		for(std::size_t i = 0; i < width; ++i){
			Generator* pt = new Generator(std::string("Generator") + n_tools::toString(i), 1000*i, randomta);
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


class Listener: public adevs::EventListener<t_event>
{
public:

	virtual void outputEvent(adevs::Event<t_event,double> x, double t){
		std::cout << "an output event at time " << t;
		Generator* proc = dynamic_cast<Generator*>(x.model);
		if(proc != nullptr)
			std::cout << "  (proc " << proc->m_name << ")";
		std::cout << '\n';

	}
	virtual void stateChange(adevs::Atomic<t_event>* model, double t){
		std::cout << "an event at time " << t << "!";
		Generator* proc = dynamic_cast<Generator*>(model);
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


const char helpstr[] = " [-h] [-t ENDTIME] [-w WIDTH] [-r] [-c COREAMT] [classic|cpdevs]\n"
	"options:\n"
	"  -h           show help and exit\n"
	"  -t ENDTIME   set the endtime of the simulation\n"
	"  -w WIDTH     the width of the high interconnect model\n"
	"  -r           use randomized processing time\n"
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
	char** argvc = argv+1;

	double eTime = 50;
	std::size_t width = 2;
	bool randTa = false;


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

	adevs::Devs<t_event>* model = new HighInterconnect(width, randTa);
#ifndef BENCHMARK
	adevs::EventListener<t_event>* listener = new Listener();
#endif
	if(isClassic){
		adevs::Simulator<t_event> sim(model);
#ifndef BENCHMARK
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
#ifndef BENCHMARK
		sim.addEventListener(listener);
#endif
		sim.execUntil(eTime);
	}
#ifndef BENCHMARK
	delete listener;
#endif
	delete model;
}
