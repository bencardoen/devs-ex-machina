/*
 * network.cpp
 *
 *  Created on: Sep 27, 2015
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
#include <vector>

#define T_GEN_TA 100
#define S_NORMAL_SIZE 100
#define S_PRIORITY_SIZE 5

const int inPort = 0;
const int outPort = 1;

struct QueueMsg
{
	typedef double t_size;
	double m_size;
	double m_startsize;
	bool m_isPriority;

	QueueMsg(t_size size, bool priority = false)
		: m_size(size), m_startsize(size), m_isPriority(priority)
	{}

	QueueMsg(const QueueMsg& other)
		: QueueMsg(other.m_startsize, other.m_isPriority)
	{}

	QueueMsg(): QueueMsg(0.0, false){}
};

inline
std::ostream& operator<<(std::ostream& out, const QueueMsg& msg)
{
	out << msg.m_size;
	if(msg.m_isPriority)
		out << " [P]";
	return out;
}

typedef std::size_t GeneratorState;	//used as seed for the random number generator.
typedef std::mt19937_64 t_randgen;
typedef adevs::PortValue<QueueMsg, int> t_event;

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
/**
 * Generates messages at a constant rate.
 */
class MsgGenerator: public adevs::Atomic<t_event>
{
private:
	static std::string getNewName()
	{
		return "MsgGenerator" + n_tools::toString(counter++);
	}
	static std::size_t counter;

	std::size_t m_priorityChance;
	double m_rate;
	double m_normalSize;
	double m_prioritySize;
	mutable t_randgen m_rand;
public:
	GeneratorState state;
	const std::string m_name;

	MsgGenerator(std::size_t chance,
		QueueMsg::t_size rate,
		QueueMsg::t_size nsize = S_NORMAL_SIZE,
		QueueMsg::t_size psize = S_PRIORITY_SIZE)
	:
		m_priorityChance(chance),
		m_rate(rate), m_normalSize(nsize), m_prioritySize(psize),
		state(counter),
		m_name(getNewName())
	{ }
	virtual ~MsgGenerator(){}

	double ta() override
	{ return m_rate; }
	
	void delta_int() override
	{ state = m_rand(); }
	
	void delta_ext(double, const adevs::Bag<t_event>&) override
	{}
	
	void delta_conf(const adevs::Bag<t_event>&) override
	{ state = m_rand(); }
	
	void output_func(adevs::Bag<t_event>& yb) override
	{
		static std::uniform_int_distribution<std::size_t> dist(1, 100);
		m_rand.seed(state);
		bool priority = dist(m_rand) <= m_priorityChance;
		QueueMsg::t_size size = priority? m_prioritySize:m_normalSize;
		yb.insert(t_event(outPort, QueueMsg(size, priority)));
	}
	
	void gc_output(adevs::Bag<t_event>&)
	{ }
	
	double lookahead() override
	{ return m_rate; }
};

std::size_t MsgGenerator::counter = 0;

class GenReceiver: public MsgGenerator
{
public:
	GenReceiver(std::size_t chance,
		QueueMsg::t_size rate,
		QueueMsg::t_size nsize = S_NORMAL_SIZE,
		QueueMsg::t_size psize = S_PRIORITY_SIZE)
	: MsgGenerator(chance, rate, nsize, psize)
	{}
	virtual ~GenReceiver(){}
};

/**
 * Simply ignores any messages it gets
 */
class Receiver: public adevs::Atomic<t_event>
{
public:
	std::string m_name;
	Receiver(): m_name("receiver")
	{}
	virtual ~Receiver(){}

	void delta_int() override
	{ }

	void delta_ext(double, const adevs::Bag<t_event>&) override
	{ }

	void delta_conf(const adevs::Bag<t_event>&) override
	{ }

	void output_func(adevs::Bag<t_event>&) override
	{ }

	void gc_output(adevs::Bag<t_event>&)
	{ }

	double ta() override
	{ return S_PRIORITY_SIZE; }

	double lookahead() override
	{ return S_PRIORITY_SIZE; }
};

struct SplitterState
{
	std::vector<QueueMsg> m_msgs;
	std::size_t seed;
	QueueMsg::t_size m_timeleft;

	SplitterState(): seed(0), m_timeleft(0)
	{}
};

inline
std::ostream& operator<<(std::ostream& out, const SplitterState& msg)
{
	out << "msgs queued: " <<  msg.m_msgs.size() << ",  ";
	out << "seed: " <<  msg.seed << ",  ";
	out << "time left: " <<  msg.m_timeleft;
	return out;
}

class Splitter: public adevs::Atomic<t_event>
{
private:
	double m_rate;
	std::size_t m_size;
	mutable std::uniform_int_distribution<std::size_t> m_dist;
	mutable t_randgen m_rand;
public:
	SplitterState state;
	std::string m_name;

	Splitter(std::size_t size, QueueMsg::t_size rate = S_NORMAL_SIZE)
	: m_rate(rate), m_size(size), m_dist(0, m_size-1), m_name("splitter")
	 {
		state.m_timeleft = rate;
	 }
	virtual ~Splitter(){}

	void delta_int() override
	{
		state.m_timeleft = m_rate;
		state.m_msgs.clear();
		state.seed = m_rand();
	}

	void delta_ext(double e, const adevs::Bag<t_event>& xb) override
	{
		if(state.m_msgs.size())
			state.m_timeleft -= e;
		else
			state.m_timeleft = m_rate;
		state.seed = m_rand();
		state.m_msgs.reserve(state.m_msgs.size() + xb.size());
		for(const t_event& ev: xb){
			state.m_msgs.push_back(ev.value);
		}
	}

	void delta_conf(const adevs::Bag<t_event>& xb) override
	{
		state.m_timeleft = m_rate;
		state.m_msgs.clear();
		state.seed = m_rand();
		state.m_msgs.reserve(xb.size());
		for(const t_event& ev: xb){
			state.m_msgs.push_back(ev.value);
		}
	}

	void output_func(adevs::Bag<t_event>& yb) override
	{
		m_rand.seed(state.seed);
		for(const QueueMsg& qmsg: state.m_msgs){
			std::size_t port = m_dist(m_rand);
			yb.insert(t_event(port, qmsg));
		}
	}

	void gc_output(adevs::Bag<t_event>&)
	{ }

	double ta() override
	{ return state.m_msgs.size()? state.m_timeleft: m_rate; }

	double lookahead() override
	{ return S_PRIORITY_SIZE; }
};

struct ServerState
{
	std::deque<QueueMsg> m_msgs;
	std::deque<QueueMsg> m_priorityMsgs;
	double m_timeElapsed;

	ServerState(): m_timeElapsed(0.0){}
};

inline
std::ostream& operator<<(std::ostream& out, const ServerState& msg)
{
	out << "p msgs queued: " <<  msg.m_priorityMsgs.size() << ",  ";
	out << "n msgs queued: " <<  msg.m_msgs.size() << ",  ";
	out << "time elapsed: " <<  msg.m_timeElapsed;
	return out;
}

class Server: public adevs::Atomic<t_event>
{
private:
	double m_rate;	//generator rate
	double m_maxSize;
public:
	ServerState state;
	std::string m_name;
	Server(QueueMsg::t_size rate, double maxSize = 100)
	: m_rate(rate), m_maxSize(maxSize), m_name("Server")
	{}
	virtual ~Server(){}

	void delta_int() override
	{
		double i = 0;
		auto piter = state.m_priorityMsgs.begin();
		auto niter = state.m_msgs.begin();
		double size = m_maxSize*m_rate;
		state.m_timeElapsed = m_rate;
		while(true) {
			if(piter != state.m_priorityMsgs.end()){
				if(i + piter->m_size <= size){
					piter = state.m_priorityMsgs.erase(piter);
					i += piter->m_size;
					continue;
				}
				break;
			} else if(niter != state.m_msgs.end()){
				if(i + niter->m_size <= size){
					niter = state.m_msgs.erase(niter);
					i += niter->m_size;
					continue;
				}
				break;
			}
			break;
		}
	}

	void delta_ext(double e, const adevs::Bag<t_event>& xb) override
	{
		state.m_timeElapsed = e;
		for(const t_event& ptr: xb){
			if(ptr.value.m_isPriority)
				state.m_priorityMsgs.push_back(ptr.value);
			else
				state.m_msgs.push_back(ptr.value);
		}
	}

	void delta_conf(const adevs::Bag<t_event>& xb) override
	{
		double i = 0;
		auto piter = state.m_priorityMsgs.begin();
		auto niter = state.m_msgs.begin();
		double size = m_maxSize*m_rate;
		state.m_timeElapsed = m_rate;
		while(true) {
			if(piter != state.m_priorityMsgs.end()){
				if(i + piter->m_size <= size){
					piter = state.m_priorityMsgs.erase(piter);
					i += piter->m_size;
					continue;
				}
				break;
			} else if(niter != state.m_msgs.end()){
				if(i + niter->m_size <= size){
					niter = state.m_msgs.erase(niter);
					i += niter->m_size;
					continue;
				}
				break;
			}
			break;
		}
		for(const t_event& ptr: xb){
			if(ptr.value.m_isPriority)
				state.m_priorityMsgs.push_back(ptr.value);
			else
				state.m_msgs.push_back(ptr.value);
		}
	}

	void output_func(adevs::Bag<t_event>& yb) override
	{
		double i = 0;
		auto piter = state.m_priorityMsgs.begin();
		auto niter = state.m_msgs.begin();
		double size = m_maxSize*state.m_timeElapsed;
		while(true) {
			if(piter != state.m_priorityMsgs.end()){
				if(i + piter->m_size <= size){
					yb.insert(t_event(outPort, QueueMsg(*piter)));
					++piter;
					i += piter->m_size;
					continue;
				}
				break;
			}
			if(niter != state.m_msgs.end()){
				if(i + niter->m_size <= size){
					yb.insert(t_event(outPort, QueueMsg(*niter)));
					++niter;
					i += niter->m_size;
					continue;
				}
				break;
			}
			break;
		}
	}

	void gc_output(adevs::Bag<t_event>&)
	{ }

	double ta() override
	{
		if(state.m_priorityMsgs.size())
			return state.m_priorityMsgs.front().m_size;
		if(state.m_msgs.size())
			return state.m_msgs.front().m_size;
		return m_rate;
	}

	double lookahead() override
	{ return m_rate; }

};

class SingleServerNetwork: public adevs::Digraph<QueueMsg>
{
public:
	std::string m_name;
	std::vector<adevs::Atomic<t_event>*> m_components;
	SingleServerNetwork(std::size_t numGenerators, std::size_t splitterSize, std::size_t priorityChance,
		QueueMsg::t_size rate,
		QueueMsg::t_size nsize = S_NORMAL_SIZE,
		QueueMsg::t_size psize = S_PRIORITY_SIZE)
	: m_name("SingleServerNetwork")
	{
		QueueMsg::t_size k = numGenerators*nsize;
		Server* server = new Server(rate, k);
		Splitter* splitter = new Splitter(splitterSize, nsize);
		Receiver* receiver = new Receiver();
		add(server);
		add(splitter);
		add(receiver);
		m_components.push_back(server);
		m_components.push_back(splitter);
		m_components.push_back(receiver);
		for(std::size_t i = 0; i < numGenerators; ++i){
			MsgGenerator* gen = new MsgGenerator(priorityChance, rate, nsize, psize);
			add(gen);
			m_components.push_back(gen);
			couple(gen, outPort, server, inPort);
		}
		couple(server, outPort, splitter, inPort);
		for(std::size_t i = 0; i < splitterSize; ++i)
			couple(splitter, i, receiver, inPort);
	}
	virtual ~SingleServerNetwork(){}
};

class FeedbackServerNetwork: public adevs::Digraph<QueueMsg>
{
public:
	std::string m_name;
	std::vector<adevs::Atomic<t_event>*> m_components;
	FeedbackServerNetwork(std::size_t numGenerators, std::size_t priorityChance,
		QueueMsg::t_size rate,
		QueueMsg::t_size nsize = S_NORMAL_SIZE,
		QueueMsg::t_size psize = S_PRIORITY_SIZE)
	: m_name("FeedbackServerNetwork")
	{
		QueueMsg::t_size k = numGenerators*nsize;
		Server* server = new Server(rate, k);
		Splitter* splitter = new Splitter(numGenerators, nsize);
		add(server);
		add(splitter);
		m_components.push_back(server);
		m_components.push_back(splitter);
		couple(server, outPort, splitter, inPort);
		for(std::size_t i = 0; i < numGenerators; ++i){
			MsgGenerator* gen = new MsgGenerator(priorityChance, rate, nsize, psize);
			add(gen);
			m_components.push_back(gen);
			couple(gen, outPort, server, inPort);
			couple(splitter, i, gen, inPort);
		}
	}
	virtual ~FeedbackServerNetwork(){}
};


class Listener: public adevs::EventListener<t_event>
{
public:

	virtual void outputEvent(adevs::Event<t_event,double> x, double t){
		std::cout << "an output event at time " << t;
		MsgGenerator* gen = dynamic_cast<MsgGenerator*>(x.model);
		Receiver* rec = gen? nullptr: dynamic_cast<Receiver*>(x.model);
		GenReceiver* genrec = rec? nullptr: dynamic_cast<GenReceiver*>(x.model);
		Splitter* split = genrec? nullptr: dynamic_cast<Splitter*>(x.model);
		Server* server = split? nullptr: dynamic_cast<Server*>(x.model);
		if(gen != nullptr)
			std::cout << "  " << gen->m_name << " created some output: " << x.value.value;
		else if(rec != nullptr)
			std::cout << "  " << rec->m_name << " created some output: " << x.value.value;
		else if(genrec != nullptr)
			std::cout << "  " << genrec->m_name << " created some output: " << x.value.value;
		else if(split != nullptr)
			std::cout << "  " << split->m_name << " created some output: " << x.value.value << " on port " << x.value.port;
		else if(server != nullptr)
			std::cout << "  " << server->m_name << " created some output: " << x.value.value;
		std::cout << '\n';

	}
	virtual void stateChange(adevs::Atomic<t_event>* model, double t){
		std::cout << "an event at time " << t << "!";
		MsgGenerator* gen = dynamic_cast<MsgGenerator*>(model);
		Receiver* rec = gen? nullptr: dynamic_cast<Receiver*>(model);
		GenReceiver* genrec = rec? nullptr: dynamic_cast<GenReceiver*>(model);
		Splitter* split = genrec? nullptr: dynamic_cast<Splitter*>(model);
		Server* server = split? nullptr: dynamic_cast<Server*>(model);
		if(gen != nullptr)
			std::cout << "  " << gen->m_name << " did something, probably. State: " << gen->state;
		else if(rec != nullptr)
			std::cout << "  " << rec->m_name << " did something, probably";
		else if(genrec != nullptr)
			std::cout << "  " << genrec->m_name << " did something, probably. State: " << genrec->state;
		else if(split != nullptr)
			std::cout << "  " << split->m_name << " did something, probably. State: " << split->state;
		else if(server != nullptr)
			std::cout << "  " << server->m_name << " did something, probably. State: " << server->state;
		std::cout << '\n';
	}

	virtual ~Listener(){}
};

void allocate(std::size_t numCores, MsgGenerator* p){
	static int count = 0;
	if(p == nullptr)
		return;
	std::size_t value = 0;
	if(numCores < 2)
		value = 0;
	else if(numCores == 2)
		value = 1;
	else
		value = 1 + (count++)%(numCores-1);
	if(value >= numCores)
		value = numCores-1;
	p->setProc(value);
}
void allocate(std::size_t numCores, SingleServerNetwork* d)
{
	for(auto& ptr: d->m_components){
		MsgGenerator* gen = dynamic_cast<MsgGenerator*>(ptr);
		if(gen)
			allocate(numCores, gen);
		else
			d->setProc(0);
	}
}
void allocate(std::size_t numCores, FeedbackServerNetwork* d)
{
	for(auto& ptr: d->m_components){
		MsgGenerator* gen = dynamic_cast<MsgGenerator*>(ptr);
		if(gen)
			allocate(numCores, gen);
		else
			d->setProc(0);
	}
}

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

	double eTime = 5000.0;
	std::size_t width = 2;
	std::size_t priority = 10;
	bool feedback = false;

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
				hasError = true;
			}
			break;
		case optETime:
			++i;
			if(i < argc){
				eTime = toData<double>(std::string(*(++argvc)));
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

	FeedbackServerNetwork* fmodel = feedback? new FeedbackServerNetwork(width, priority, 100) : nullptr;
	SingleServerNetwork* smodel = feedback? nullptr : new SingleServerNetwork(width, width, priority, 100);
	adevs::Digraph<QueueMsg>* model = feedback?
						 static_cast<adevs::Digraph<QueueMsg>*>(fmodel)
						:static_cast<adevs::Digraph<QueueMsg>*>(smodel);

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
		adevs::ParSimulator<t_event>* sim = nullptr;
		if(feedback){
			allocate(coreAmt, fmodel);
			adevs::LpGraph lpg;
			for(std::size_t i = 1; i < coreAmt; ++i)
				lpg.addEdge(i-1, i);
			sim = new adevs::ParSimulator<t_event>(model, lpg);
		} else {
			allocate(coreAmt, smodel);
			sim = new adevs::ParSimulator<t_event>(model);
		}
#ifndef BENCHMARK
		sim->addEventListener(listener);
#endif
		sim->execUntil(eTime);
		delete sim;
	}
#ifndef BENCHMARK
	delete listener;
#endif
	delete model;
}
