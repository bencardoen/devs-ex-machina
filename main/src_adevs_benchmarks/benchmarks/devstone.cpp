/*
 * devstone.cpp
 *
 *  Created on: Aug 17, 2015
 *      Author: Devs Ex Machina
 */

#include <adevs.h>
#include <iostream>
#include <cstdlib>	//std::size_t
#include <cstring>
#include <sstream>
#include <string>
#include <limits>
#include <random>
#include "common.h"
#include "../../main/src/tools/frandom.h"
#include "../../main/src/tools/stringtools.h"

#ifdef FPTIME
#define T_0 0.0
#define T_1 0.01
#define T_STEP 0.01
#define T_50 0.5
#define T_75 0.75
#define T_100 1.0
#define T_125 1.25
double T_INF = std::numeric_limits<double>::max();
# else
#define T_0 0.0
#define T_1 1.0
#define T_STEP 1.0
#define T_50 50.0
#define T_75 75.0
#define T_100 100.0
#define T_125 125.0
double T_INF = std::numeric_limits<double>::max();
#endif
//#define T_INF DBL_MAX

typedef adevs::PortValue<std::size_t, int> t_event;
typedef std::size_t t_counter;

#ifdef FRNG
	typedef n_tools::n_frandom::marsaglia_xor_64_s t_randgen;
#else
	typedef std::mt19937_64 t_randgen;
#endif
typedef boost::random::taus88 t_seedrandgen;    //this random generator will be used to generate the initial seeds
//it MUST be diferent from the regular t_randgen
static_assert(!std::is_same<t_randgen, t_seedrandgen>::value, "The rng for the seed can't be the same random number generator as the one for he random events.");


const t_counter inf = std::numeric_limits<t_counter>::max();
const t_event empty = t_event(0, -1);

template<typename T>
constexpr T roundTo(T val, T gran)
{
	return std::round(val/gran)*gran;
}

bool operator==(const t_event& lhs, const t_event& rhs){
	return (lhs.port == rhs.port && lhs.value == rhs.value);
}

bool operator!=(const t_event& lhs, const t_event& rhs){
	return !(lhs == rhs);
}

const int inPort = 0;
const int outPort = 1;

class Processor: public adevs::Atomic<t_event>
{
private:
	//state
	t_counter m_event1_counter;
	t_event m_event1;
	std::vector<t_event> m_queue;

	//constant
	const bool m_randta;
	mutable t_randgen m_rand;
public:
	const std::size_t m_counter;
	Processor(bool randta, std::size_t num, std::size_t startSeed):
		adevs::Atomic<t_event>(),
		m_event1_counter(inf),
		m_event1(empty),
		m_randta(randta),
		m_counter(num)
	{
	    m_rand.seed(startSeed);
//		std::cout << "processor " << m_counter << " was born!\n";
	}

	/// Internal transition function.
	void delta_int()
	{
//		std::cout << "proc " << m_counter << " internal\n";
		if(m_queue.empty()) {
			m_event1_counter = inf;
			m_event1 = empty;
		} else {
			m_event1 = m_queue.back();
			m_queue.pop_back();
            m_event1_counter = (m_randta) ? getProcTime(m_event1.value) : T_100;
		}
	}
	/// External transition function.
	void delta_ext(double e, const adevs::Bag<t_event>& xb)
	{
//		std::cout << "proc " << m_counter << " external\n";
		m_event1_counter = (m_event1_counter == inf)? inf : (m_event1_counter - e);
		t_event ev = *(xb.begin());
		if(m_event1 != empty) {
			m_queue.push_back(ev);
		} else {
			m_event1 = ev;
			m_event1_counter = (m_randta) ? getProcTime(m_event1.value) : T_100;
		}
	}
	/// Confluent transition function.
	void delta_conf(const adevs::Bag<t_event>& xb)
	{
//		std::cout << "proc " << m_counter << " confluent\n";
//		std::cout << " m_event1";
		m_event1_counter = T_0;
		//We can only have 1 message
		bool replacedMessage = false;
		if(m_queue.empty()) {
			m_event1_counter = inf;
			m_event1 = empty;
		} else {
			m_event1_counter = (m_randta) ? getProcTime(m_event1.value) : T_100;
			m_event1 = m_queue.back();
			m_queue.pop_back();
			replacedMessage = true;
		}
		t_event ev = *(xb.begin());
		if(m_event1 != empty || replacedMessage) {
			m_queue.push_back(ev);
		} else {
			m_event1 = ev;
			m_event1_counter = (m_randta) ? getProcTime(m_event1.value) : T_100;
		}
	}
	/// Output function.
	void output_func(adevs::Bag<t_event>& yb)
	{
		if(m_event1_counter != inf)
			yb.insert(t_event(outPort, m_event1.value));
	}
	/// Time advance function.
	double ta()
	{
//		std::cout << "proc " << m_counter << " ta = " << ((m_event1_counter == inf)? procCounter*10: m_event1_counter) << "\n";
		if(m_event1_counter == inf)
			return T_INF;
		return m_event1_counter;
	}
	/// Garbage collection
	void gc_output(adevs::Bag<t_event>&)
	{
	}

	double lookahead()
	{
		if(m_event1_counter == inf)
			return T_1;	//does not allow lookahead of 0, so we'll have to cheat.
		return T_100;
	}

	double getProcTime(size_t) const
	{
#ifdef FPTIME
        std::uniform_real_distribution<double> dist(T_75, T_125);
        double ta = roundTo(dist(m_rand), T_STEP);
#else
        std::uniform_int_distribution<std::size_t> dist(T_75, T_125);
        double ta = double(dist(m_rand));
#endif
		return ta;
	}
};

class Generator: public adevs::Atomic<t_event>
{
	void delta_int()
	{
//		std::cout << "gen internal\n";
	}
	/// External transition function.
	void delta_ext(double, const adevs::Bag<t_event>&)
	{
	}
	/// Confluent transition function.
	void delta_conf(const adevs::Bag<t_event>&)
	{
	}
	/// Output function.
	void output_func(adevs::Bag<t_event>& yb)
	{
//		std::cout << "gen output\n";
		yb.insert(t_event(outPort, 1));
	}
	/// Time advance function.
	double ta()
	{
		return T_100;
	}
	/// Garbage collection
	void gc_output(adevs::Bag<t_event>&)
	{
	}

	double lookahead()
	{
		return T_INF;
	}
};

class CoupledRecursion: public adevs::Digraph<std::size_t>
{
public:
	std::vector<Component*> m_components;
	/// Assigns the model component set to c
	CoupledRecursion(std::size_t width, std::size_t depth, bool randomta, std::size_t& num, t_seedrandgen& getSeed):
		adevs::Digraph<std::size_t>()
	{
		assert(width > 0 && "The width must me at least 1.");
		if(depth > 1) {
	        for(std::size_t i = 0; i < width; ++i){
                Component* c = new CoupledRecursion(1, depth-1, randomta, num++, getSeed);
                m_components.push_back(c);
                add(c);
	        }
		} else {
            for(std::size_t i = 0; i < width; ++i){
                Component* c = new Processor(randomta, num++, getSeed());
                m_components.push_back(c);
                add(c);
            }
		}
		couple(this, inPort, m_components.front(), inPort);
		auto i = m_components.begin();
		auto i2 = i + 1;
		for(; i2 != m_components.end(); ++i,++i2){
			couple(*i, outPort, *i2, inPort);
		}
		couple(m_components.back(), outPort, this, outPort);
	}

	double lookahead()
	{
		double minv = T_INF;
		for(auto i:m_components)
			minv = std::min(i->lookahead(), minv);
		return minv;
	}
};


class DEVSTone: public adevs::Digraph<std::size_t>
{
public:
	Component* m_gen;
	Component* m_proc;
	std::size_t num = 0;
	DEVSTone(std::size_t width, std::size_t depth, bool randomta, t_seedrandgen& getSeed):
		m_gen(new Generator()), m_proc(new CoupledRecursion(width, depth, randomta, num, getSeed))
	{
		add(m_gen);
		add(m_proc);
		couple(m_gen, outPort, m_proc, inPort);
	}

	double lookahead()
	{
		return std::min(m_gen->lookahead(), m_proc->lookahead());
	}
};


struct DevstoneName {
        static std::string eval(adevs::Devs<t_event, double>* model) {
            Processor* proc = dynamic_cast<Processor*>(model);
            if(proc != nullptr) return std::string("Processor ") + n_tools::toString(proc->m_counter);
            Generator* gen = dynamic_cast<Generator*>(model);
            if(gen != nullptr) return "Generator";
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

void allocate(std::size_t numCores, std::size_t pCount, Processor* p){
	if(p != nullptr){
        double blocksize = (double) (pCount + 1) / (double) numCores;
        size_t core = (1+p->m_counter)/blocksize;
		p->setProc(core);
//		std::cout << "assigned processor_" << p->m_counter << " to core " << core << '\n';
	}
}
void allocate(std::size_t, std::size_t, Generator* g){
	g->setProc(0);
}
void findProcessors(std::vector<Processor*>& procs, adevs::Devs<t_event>* c){
	Processor* p = dynamic_cast<Processor*>(c);
	if(p != nullptr){
		procs.push_back(p);
//		std::cout << "found processor " << p->m_counter << '\n';
	}
	else {
		CoupledRecursion* r = dynamic_cast<CoupledRecursion*>(c);
		if(r != nullptr){
			for(adevs::Devs<t_event>* i:r->m_components)
				findProcessors(procs, i);
		}
	}
}
void allocate(std::size_t numcores, DEVSTone* d){
	allocate(0, 0, dynamic_cast<Generator*>(d->m_gen));
	//get a list of all processors
	std::vector<Processor*> procs;
	findProcessors(procs, d->m_proc);
	for(Processor* i: procs)
		allocate(numcores, procs.size(), i);
}


const char helpstr[] = " [-h] [-t ENDTIME] [-w WIDTH] [-d DEPTH] [-r] [-S seed] [-c COREAMT] [classic|cpdevs]\n"
	"options:\n"
	"  -h           show help and exit\n"
	"  -t ENDTIME   set the endtime of the simulation\n"
	"  -w WIDTH     the with of the devstone model\n"
	"  -d DEPTH     the depth of the devstone model\n"
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
	const char optDepth = 'd';
	const char optHelp = 'h';
	const char optRand = 'r';
	const char optCores = 'c';
    const char optSeed = 'S';
	char** argvc = argv+1;

	double eTime = 50;
	std::size_t width = 2;
	std::size_t depth = 3;
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
		case optDepth:
			++i;
			if(i < argc){
				depth = toData<std::size_t>(std::string(*(++argvc)));
			} else {
				std::cout << "Missing argument for option -" << optDepth << '\n';
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
    t_seedrandgen getSeed;
    getSeed.seed(initialSeed);
	DEVSTone* model = new DEVSTone(width, depth, randTa, getSeed);
#ifdef USE_STAT
#undef USE_STAT
#endif
#ifdef USE_STAT
    #define USE_LISTENER
    adevs::EventListener<t_event>* listener = new OutputCounter<t_event>();
#else
#ifndef BENCHMARK
    #define USE_LISTENER
    adevs::EventListener<t_event>* listener = new Listener<t_event, DevstoneName>();
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
		allocate(coreAmt, model);
		adevs::LpGraph lpg;
		for(std::size_t i = 1; i < coreAmt; ++i)
			lpg.addEdge(i-1, i);
		adevs::ParSimulator<t_event> sim(model, lpg);
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
