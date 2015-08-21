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

#define T_0 0.0
#define T_50 0.5
#define T_75 0.75
#define T_100 1.0
#define T_INF DBL_MAX

typedef adevs::PortValue<std::size_t, int> t_event;
typedef std::size_t t_counter;

const t_counter inf = std::numeric_limits<t_counter>::max();
const t_event empty = t_event(0, -1);

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
	static std::size_t procCounter;
	//state
	t_counter m_event1_counter;
	t_event m_event1;
	std::vector<t_event> m_queue;

	//constant
	const bool m_randta;
public:
	const std::size_t m_counter;
	Processor(bool randta = false):
		adevs::Atomic<t_event>(),
		m_event1_counter(inf),
		m_event1(empty),
		m_randta(randta),
		m_counter(procCounter++)
	{
		std::cout << "processor " << m_counter << " was born!\n";
	}

	/// Internal transition function.
	void delta_int()
	{
//		std::cout << "proc " << m_counter << " internal\n";
		if(m_queue.empty()) {
			m_event1_counter = inf;
			m_event1 = t_event(0, 0);
		} else {
			m_event1_counter = (m_randta) ? (T_75 + (rand() / (RAND_MAX / T_50))) : T_100;
			m_event1 = m_queue.back();
			m_queue.pop_back();
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
			m_event1_counter = (m_randta) ? (T_75 + (rand() / (RAND_MAX / T_50))) : T_100;
		}
	}
	/// Confluent transition function.
	void delta_conf(const adevs::Bag<t_event>& xb)
	{
//		std::cout << "proc " << m_counter << " confluent\n";
		m_event1_counter = T_0;
		//We can only have 1 message
		if(m_queue.empty()) {
			m_event1_counter = inf;
			m_event1 = t_event(0, 0);
		} else {
			m_event1_counter = (m_randta) ? (T_75 + (rand() / (RAND_MAX / T_50))) : T_100;
			m_event1 = m_queue.back();
			m_queue.pop_back();
		}
		t_event ev = *(xb.begin());
		if(m_event1 != empty) {
			m_queue.push_back(ev);
		} else {
			m_event1 = ev;
			m_event1_counter = (m_randta) ? (T_75 + (rand() / (RAND_MAX / T_50))) : T_100;
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
			return (1);	//regularly wake up
		return m_event1_counter;
	}
	/// Garbage collection
	void gc_output(adevs::Bag<t_event>&)
	{
	}
};

std::size_t Processor::procCounter = 0;

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
};

class CoupledRecursion: public adevs::Digraph<std::size_t>
{
public:
	/// Assigns the model component set to c
	CoupledRecursion(std::size_t width, std::size_t depth, bool randomta):
		adevs::Digraph<std::size_t>()
	{
		assert(width > 0 && "The width must me at least 1.");
		std::vector<Component*> components;
		if(depth > 1)
		{
			Component* c = new CoupledRecursion(width, depth-1, randomta);
			components.push_back(c);
			add(c);
		}
		for(std::size_t i = 0; i < width; ++i){
			Component* c = new Processor(randomta);
			components.push_back(c);
			add(c);
		}
		couple(this, inPort, components.front(), inPort);
		auto i = components.begin();
		auto i2 = i + 1;
		for(; i2 != components.end(); ++i,++i2){
			couple(*i, outPort, *i2, inPort);
		}
		couple(components.back(), outPort, this, outPort);
	}
};


class DEVSTone: public adevs::Digraph<std::size_t>
{
public:
	DEVSTone(std::size_t width, std::size_t depth, bool randomta)
	{
		Component* gen = new Generator();
		Component* proc = new CoupledRecursion(width, depth, randomta);
		add(gen);
		add(proc);
		couple(gen, outPort, proc, inPort);
	}
};

class Listener: public adevs::EventListener<t_event>
{
public:

	virtual void outputEvent(adevs::Event<t_event,double> x, double t){
		std::cout << "an output event at time " << t;
		Processor* proc = dynamic_cast<Processor*>(x.model);
		if(proc != nullptr)
			std::cout << "  (proc " << proc->m_counter << ")";
		std::cout << '\n';

	}
	virtual void stateChange(adevs::Atomic<t_event>* model, double t){
		std::cout << "an event at time " << t << "!";
		Processor* proc = dynamic_cast<Processor*>(model);
		if(proc != nullptr)
			std::cout << "  (proc " << proc->m_counter << ")";
		std::cout << '\n';
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



int main(int argc, char** argv)
{
	const char optETime = 't';
	const char optWidth = 'w';
	const char optDepth = 'd';
	const char optHelp = 'h';
	const char optRand = 'r';
	char** argvc = argv+1;

	double eTime = 50;
	std::size_t width = 2;
	std::size_t depth = 3;
	bool randTa = false;

	bool hasError = false;
	bool isClassic = true;

	for(int i = 1; i < argc; ++argvc, ++i){
		char c = getOpt(*argvc);
		if(!c){
			if(!strcmp(*argvc, "classic")){
				isClassic = true;
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
				std::cout << "Missing argument for option -" << optETime << '\n';
			}
			break;
		case optDepth:
			++i;
			if(i < argc-1){
				depth = toData<std::size_t>(std::string(*(++argvc)));
			} else {
				std::cout << "Missing argument for option -" << optETime << '\n';
			}
			break;
		case optRand:
			randTa = true;
			srand(0);
			break;
		case optHelp:
			std::cout << "usage: \n\t" << argv[0] << "[-h] [-t ENDTIME] [-w WIDTH] [-d DEPTH] [-r]\n";
			return 0;
		default:
			std::cout << "Unknown argument: " << *argvc << '\n';
			hasError = true;
			continue;
		}
	}
	if(hasError)
		return -1;

	if(isClassic){
		adevs::Devs<t_event>* model = new DEVSTone(width, depth, randTa);
		adevs::Simulator<t_event> sim(model);
		adevs::EventListener<t_event>* listener = new Listener();
		sim.addEventListener(listener);
		sim.execUntil(eTime);
		delete listener;
		delete model;
	}
}
