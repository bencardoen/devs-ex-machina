/*
 * pholdtree.cpp
 *
 *  Created on: Apr 16, 2016
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
#include <deque>
#include <cmath>
#include <type_traits>
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
typedef int t_portType;
typedef adevs::PortValue<t_payload, t_portType> t_event;
constexpr std::size_t nullDestination = std::numeric_limits<std::size_t>::max();

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
//	std::uniform_int_distribution<std::size_t> dist(0, 60000);
//	randgen.seed(event);
//	return dist(randgen);
    return randgen();
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
class PHOLDTreeProcessor: public adevs::Atomic<t_event>
{
private:
    mutable std::uniform_int_distribution<std::size_t> m_distDest;
    const double m_percentagePriority;
    mutable t_randgen m_rand;   //This object could be a global object, but then we'd need to lock it during parallel simulation.
    bool m_isRoot;
    std::size_t m_modelNumber;
    std::vector<t_portType> m_oPorts;
    std::vector<t_portType> m_iPorts;
    std::deque<EventPair> m_events;
    std::size_t m_eventsProcessed;
    std::size_t m_destination;  //the next destination
    std::size_t m_nextMessage;  //the next message

	size_t getNextDestination(size_t) const
	{
        if(m_oPorts.empty())
            return nullDestination;
        size_t chosen = m_distDest(m_rand);
        return chosen;
	}
	t_eventTime getProcTime(t_payload) const
	{
        std::uniform_real_distribution<double> dist0(0.0, 1.0);
		std::uniform_int_distribution<int> dist(int(T_100), int(T_125));
		double ta = double(dist(m_rand)); //roundTo(dist(m_rand), T_STEP);
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
	PHOLDTreeProcessor(std::string name, size_t modelNumber, double percentagePriority, std::size_t startSeed, bool isRoot = false):
		  m_percentagePriority(percentagePriority), m_isRoot(isRoot), m_modelNumber(modelNumber), m_eventsProcessed(0),
		  m_destination(nullDestination), m_nextMessage(0),
		 m_name(name)
	{
        m_rand.seed(startSeed);
        if(m_percentagePriority > 1.0 )
                throw std::logic_error("Invalid value for priority");
        if(isRoot)
            m_events.push_back(EventPair(modelNumber, getProcTime(modelNumber)));
	}

	/// Internal transition function.
	void delta_int()
	{
	    if(m_isRoot && m_events.size() == 1){
            m_eventsProcessed++;
            m_events.push_back(EventPair(m_modelNumber, getProcTime(0)));
        }
		if(m_events.size())
			m_events.pop_front();
        if(m_events.size()) {
            finalize();
        }
	}
	/// External transition function.
	void delta_ext(double e, const adevs::Bag<t_event>& xb)
	{
        bool wasEmpty = m_events.empty();
        if (!wasEmpty) {
			m_events[0].m_procTime -= e;
        }
		for (auto& msg : xb) {
			++m_eventsProcessed;
			m_events.push_back(EventPair(m_modelNumber, getProcTime(0)));
		}
        if(wasEmpty) {
            finalize();
        }
	}
	/// Confluent transition function.
	void delta_conf(const adevs::Bag<t_event>& xb)
	{
	    bool wasEmpty = m_events.empty();
	    if (!wasEmpty) {
	        m_events.pop_front();
	    }
        for (auto& msg : xb) {
            ++m_eventsProcessed;
//            t_payload payload = msg.value + m_eventsProcessed;
            m_events.push_back(EventPair(m_modelNumber, getProcTime(0)));
        }
        if(wasEmpty) {
            finalize();
        }
	}
	/// Output function.
	void output_func(adevs::Bag<t_event>& yb)
	{
        static std::size_t counter(0);
		if (!m_events.empty()) {
			EventPair& i = m_events[0];
			size_t dest = m_destination;
	        //don't do anything if the destination is the nulldestination
	        if(dest == nullDestination) {
	            return;
	        }
			size_t r = m_nextMessage;
			yb.insert(t_event(dest, r));
		} else {
		    std::cout << "output function called on empty event list for the " << (++counter) << " time\n";
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

	t_portType startConnection()
	{
	    m_oPorts.push_back(m_oPorts.size());
	    m_distDest = std::uniform_int_distribution<std::size_t>(0, m_oPorts.size()-1u);
	    return m_oPorts.back();
	}

    t_portType endConnection()
    {
        m_iPorts.push_back(m_iPorts.size());
        return m_iPorts.back();
    }

    std::size_t eventsLeft()
    {
        return m_events.size();
    }

    void finalize()
    {
        if(m_events.size()) {
            //schedule the very first item, otherwise, don't schedule anything
            m_destination = getNextDestination(m_events.front().m_modelNumber);
            m_nextMessage = getRand(m_events.front().m_modelNumber, m_rand);
        }
    }
};

struct PHOLDTreeConfig
{
    std::size_t numChildren;    //number of children per node
    std::size_t depth;          //depth of the tree
    double percentagePriority;  //priority message spawn chance
    bool spawnAtRoot;           //only root node can spawn
    bool doubleLinks;           //make links double
    bool circularLinks;         //make children a circular linked list
    bool depthFirstAlloc;       //do a depth first allocation instead of a breadth first one.
    size_t initialSeed;         //the initial seed from which the model rng's are initialized. The default is 42, because some rng can't handle seed 0
    t_seedrandgen getSeed;      //the random number generator for getting the new seeds.
    //other configuration?
    PHOLDTreeConfig(): numChildren(0u), depth(0), percentagePriority(0.1), spawnAtRoot(true),
                        doubleLinks(false), circularLinks(false), depthFirstAlloc(false), initialSeed(0)
    {}
};

std::size_t numCounter = 0u;

class PHOLDTree: public adevs::Digraph<t_payload, int>
{
protected:
    std::vector<t_portType> m_oPorts;
    std::vector<t_portType> m_iPorts;

    PHOLDTree(PHOLDTreeConfig& config, std::size_t depth, std::size_t& itemNum): m_name(std::string("PHOLDTree_") + n_tools::toString(itemNum++)) {
        m_children.reserve(config.numChildren + 1);

        //create main child
        bool isRootSpawn = (depth == config.depth) && config.spawnAtRoot;
        if(isRootSpawn)
            config.getSeed.seed(config.initialSeed);
        PHOLDTreeProcessor* mainChild = new PHOLDTreeProcessor("Processor_" + n_tools::toString(itemNum),
                                                                 itemNum, config.percentagePriority, config.getSeed(), isRootSpawn);

        add((Component*)mainChild);
        m_children.push_back(mainChild);
        ++itemNum;
        std::size_t counterMax = config.numChildren + (config.circularLinks? 0:-1);

        //spawn children
        if(depth == 0) {
            //create all simple children
            for(std::size_t i = 0; i < config.numChildren; ++i) {
                PHOLDTreeProcessor * itemPtr = new PHOLDTreeProcessor("Processor_" + n_tools::toString(itemNum),
                                                                         itemNum, config.percentagePriority, config.getSeed());
                ++itemNum;
                add((Component*)itemPtr);
                m_children.push_back((Component*)itemPtr);
                //can't connect main & child here
            }
            //interconnect all children
            for(std::size_t i = 0; i < counterMax; ++i) {
                Component* cmp1 = m_children[i+1];
                Component* cmp2 = m_children[(i+1)%config.numChildren + 1];
                t_portType ptr1a = ((PHOLDTreeProcessor*)cmp1)->startConnection();
                t_portType ptr2a = ((PHOLDTreeProcessor*)cmp2)->endConnection();
                couple(cmp1, ptr1a, cmp2, ptr2a);
#ifndef BENCHMARK
                std::cerr << ((PHOLDTreeProcessor*)cmp1)->m_name << ":" << ptr1a << " -> " << ((PHOLDTreeProcessor*)cmp2)->m_name << ":" << ptr2a << '\n';
#endif
                if(config.doubleLinks){
                    t_portType ptr2b = ((PHOLDTreeProcessor*)cmp2)->startConnection();
                    t_portType ptr1b = ((PHOLDTreeProcessor*)cmp1)->endConnection();
                    couple(cmp2, ptr2b, cmp1, ptr1b);
#ifndef BENCHMARK
                    std::cerr << ((PHOLDTreeProcessor*)cmp2)->m_name << ":" << ptr2b << " -> " << ((PHOLDTreeProcessor*)cmp1)->m_name << ":" << ptr1b << '\n';
#endif
                }
            }
            //connect main child with other children
            for(std::size_t i = 0; i < config.numChildren; ++i) {
                Component* cmp1 = m_children[0];
                Component* cmp2 = m_children[i + 1];
                t_portType ptr1a = mainChild->startConnection();
                t_portType ptr2a = ((PHOLDTreeProcessor*)cmp2)->endConnection();
                couple(cmp1, ptr1a, cmp2, ptr2a);
#ifndef BENCHMARK
                std::cerr << ((PHOLDTreeProcessor*)cmp1)->m_name << ":" << ptr1a << " -> " << ((PHOLDTreeProcessor*)cmp2)->m_name << ":" << ptr2a << '\n';
#endif
                if(config.doubleLinks){
                    t_portType ptr2b = ((PHOLDTreeProcessor*)cmp2)->startConnection();
                    t_portType ptr1b = mainChild->endConnection();
                    couple(cmp2, ptr2b, cmp1, ptr1b);
#ifndef BENCHMARK
                    std::cerr << ((PHOLDTreeProcessor*)cmp2)->m_name << ":" << ptr2b << " -> " << ((PHOLDTreeProcessor*)cmp1)->m_name << ":" << ptr1b << '\n';
#endif
                }
                ((PHOLDTreeProcessor*)cmp2)->finalize();
            }
        } else {
            //create all simple children
            for(std::size_t i = 0; i < config.numChildren; ++i) {
                PHOLDTree* itemPtr = new PHOLDTree(config, depth - 1, itemNum);
                ++itemNum;
                add((Component*)itemPtr);
                m_children.push_back((Component*)itemPtr);
                //connect main & child
                t_portType ptr1a = mainChild->startConnection();
                t_portType ptr2a = itemPtr->endConnection();
                couple((Component*)mainChild, ptr1a, (Component*)itemPtr, ptr2a);
#ifndef BENCHMARK
                std::cerr << mainChild->m_name << ":" << ptr1a << " -> " << itemPtr->m_name << ":" << ptr2a << '\n';
#endif
                if(config.doubleLinks){
                    t_portType ptr2b = itemPtr->startConnection();
                    t_portType ptr1b = mainChild->endConnection();
                    couple((Component*)itemPtr, ptr2b, (Component*)mainChild, ptr1b);
#ifndef BENCHMARK
                    std::cerr << itemPtr->m_name << ":" << ptr2b << " -> " << mainChild->m_name << ":" << ptr1b << '\n';
#endif
                }
            }
            //interconnect all children
            for(std::size_t i = 0; i < counterMax; ++i) {
                // + 1 as the first child is the main child, which is not a PHOLDTree!
                Component* child1 = m_children[i+1];
                Component* child2 = m_children[(i+1)%config.numChildren + 1];
                t_portType ptr1a = ((PHOLDTree*)child1)->startConnection();
                t_portType ptr2a = ((PHOLDTree*)child2)->endConnection();
                couple(child1, ptr1a, child2, ptr2a);
#ifndef BENCHMARK
                std::cerr << ((PHOLDTree*)child1)->m_name << ":" << ptr1a << " -> " << ((PHOLDTree*)child2)->m_name << ":" << ptr2a << '\n';
#endif
                if(config.doubleLinks){
                    t_portType ptr2b = ((PHOLDTree*)child2)->startConnection();
                    t_portType ptr1b = ((PHOLDTree*)child1)->endConnection();
                    couple(child2, ptr2b, child1, ptr1b);
#ifndef BENCHMARK
                    std::cerr << ((PHOLDTree*)child2)->m_name << ":" << ptr2b << " -> " << ((PHOLDTree*)child1)->m_name << ":" << ptr1b << '\n';
#endif
                }
            }
            //finalize subcomponents
            for(std::size_t i = 0; i < config.numChildren; ++i) {
                // + 1 as the first child is the main child, which is not a PHOLDTree!
                ((PHOLDTree*)m_children[i+1])->finalizeSetup();
            }
        }

        if(isRootSpawn)
            mainChild->finalize();
    }
public:
    std::vector<Component*> m_children;
    std::string m_name;

    PHOLDTree(PHOLDTreeConfig& config)
        : PHOLDTree(config, config.depth, numCounter)
    { }


	double lookahead()
	{
		return T_STEP;
	}


    t_portType startConnection()
    {
        m_oPorts.push_back(m_oPorts.size());
        return m_oPorts.back();
    }

    t_portType endConnection()
    {
        m_iPorts.push_back(m_iPorts.size());
        return m_iPorts.back();
    }

    void finalizeSetup()
    {
        PHOLDTreeProcessor* mainChild = ((PHOLDTreeProcessor*)m_children[0]);
        //connect main child to all my ports
        for(const t_portType& port: m_oPorts) {
            t_portType ptr1 = mainChild->startConnection();
            couple((Component*)mainChild, ptr1, (Component*) this, port);
#ifndef BENCHMARK
            std::cerr << mainChild->m_name << ":" << ptr1 << " -> " << m_name << ":" << port << '\n';
#endif
        }
        for(const t_portType& port: m_iPorts) {
            t_portType ptr1 = mainChild->endConnection();
            couple((Component*) this, port, (Component*)mainChild, ptr1);
#ifndef BENCHMARK
            std::cerr << m_name << ":" << port << " -> " << mainChild->m_name << ":" << ptr1 << '\n';
#endif
        }
        mainChild->finalize();
    }
};

void allocateTree(PHOLDTree* root, const PHOLDTreeConfig& config, std::size_t numCores) {
    //precompute number of items per core
    if(numCores < 2) return; // no multithreading, all is automatically allocated on one core.
    const std::size_t p = config.numChildren;
    const std::size_t d = config.depth+2;   //+ 2 because the tree is actually 2 layers larger because we put a PHOLDTree at level 0
    const std::size_t numItems = std::ceil(double(std::pow(p, d) - 1) / ((p - 1)*numCores));
    std::size_t curNumChildren = 0;
    int curCore = 0;
    //need breadth-first search through the entire tree
    std::deque<PHOLDTree::Component*> todoList;
    todoList.push_back(static_cast<PHOLDTree::Component*>(root));
    while(todoList.size()) {
        //get the top item
        PHOLDTree::Component* top = todoList.front();
        todoList.pop_front();
        //test if it is a PHOLDTree item
        auto treeTop = dynamic_cast<PHOLDTree*>(top);
        if(treeTop != nullptr) {
            const auto& components  = treeTop->m_children;
            if(config.depthFirstAlloc) {
                //just add everything in reverse order to the front of the todo list
                todoList.insert(todoList.begin(), components.rbegin(), components.rend());
            } else {
                //add the main child of the item
                auto mnChild = static_cast<PHOLDTreeProcessor*>(components[0]);
                mnChild->setProc(curCore);
#ifndef BENCHMARK
                std::cerr << "allocating " << mnChild->m_name << " to " << curCore << "\n";
#endif
                ++curNumChildren;
                if(curNumChildren == numItems) {
                    ++curCore;
                    curNumChildren = 0;
                }
                //add the other children to the todoList
                todoList.insert(todoList.end(), components.begin()+1, components.end());
            }
        } else if(!config.depthFirstAlloc){
            //from here on, everything must be a normal PHOLDTreeProcessor in breadth first search.
            todoList.push_front(static_cast<PHOLDTree::Component*>(top));
            while(todoList.size()) {
                PHOLDTree::Component* itop = todoList.front();
                todoList.pop_front();
                auto procItem = static_cast<PHOLDTreeProcessor*>(itop);
                procItem->setProc(curCore);
#ifndef BENCHMARK
                std::cerr << "allocating " << procItem->m_name << " to " << curCore << "\n";
#endif
                ++curNumChildren;
                if(curNumChildren == numItems) {
                    ++curCore;
                    curNumChildren = 0;
                }
            }
        } else {
            //depth first search encounter of a PHOLDTreeProcessor
            auto procItem = static_cast<PHOLDTreeProcessor*>(top);
            procItem->setProc(curCore);
#ifndef BENCHMARK
            std::cerr << "allocating " << procItem->m_name << " to " << curCore << "\n";
#endif
            ++curNumChildren;
            if(curNumChildren == numItems) {
                ++curCore;
                curNumChildren = 0;
            }
        }
    }
}


class Listener: public adevs::EventListener<t_event>
{
    double m_lastTime;
public:
    Listener(): m_lastTime(0.0){
        std::cout << "note: this Listener is not threadsafe!\n";
    }
	virtual void outputEvent(adevs::Event<t_event,double> x, double t){
	    if(t > m_lastTime) {
	        std::cout << "\n__  Current Time: " << t << "____________________\n\n\n";
	        m_lastTime = t;
	    }
		PHOLDTreeProcessor* proc = dynamic_cast<PHOLDTreeProcessor*>(x.model);
		if(proc != nullptr) {
		    std::cout << "output by: (proc " << proc->m_name << ")\n";
		}

	}
	virtual void stateChange(adevs::Atomic<t_event>* model, double t){
        if(t > m_lastTime) {
            std::cout << "\n__  Current Time: " << t << "____________________\n\n\n";
            m_lastTime = t;
        }
		PHOLDTreeProcessor* proc = dynamic_cast<PHOLDTreeProcessor*>(model);
        if(proc != nullptr) {
            std::cout << "stateChange by: (proc " << proc->m_name << ")\n";
            std::cout << "\t\tEvents left: " << proc->eventsLeft() << "\n";
            if(model->ta() == T_INF)
                std::cout << "\t\tNext scheduled internal transition at time inf\n";
            else
                std::cout << "\t\tNext scheduled internal transition at time " << t + model->ta() << '\n';
        }
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


const char helpstr[] = " [-h] [-t ENDTIME] [-n NODES] [-d depth] [-p PRIORITY] [-C] [-D] [-F] [-S seed] [-c COREAMT] [classic|cpdevs]\n"
        "options:\n"
        "  -h             show help and exit\n"
        "  -t ENDTIME     set the endtime of the simulation\n"
        "  -n NODES       number of pholdtree nodes per tree node\n"
        "  -d DEPTH       depth of the pholdtree\n"
        "  -p PRIORITY    chance of a priority event. Must be within the range [0.0, 1.0]\n"
        "  -C             Enable circular links among the children of the same root.\n"
        "  -D             Enable double links. This will allow nodes to communicate in counterclockwise order and to their parent.\n"
        "  -F             Enable depth first allocation of the nodes across the cores in multicore simulation. The default is breadth first allocation.\n"
        "  -S seed        Initial seed with which all random number generators are seeded.\n"
        "  -c COREAMT     amount of simulation cores, ignored in classic mode.\n"
        "  classic        Run single core simulation.\n"
        "  cpdevs         Run conservative parallel simulation.\n"
        "note:\n"
        "  If the same option is set multiple times, only the last value is taken.\n";
int main(int argc, char** argv)
{
    const char optETime = 't';
    const char optWidth = 'n';
    const char optDepth = 'd';
    const char optDepthFirst = 'F';
    const char optSeed = 'S';
    const char optHelp = 'h';
    const char optPriority = 'p';
    const char optCores = 'c';
    const char optDoubleLinks = 'D';
    const char optCircularLinks = 'C';
	char** argvc = argv+1;

	double eTime = 50;
    PHOLDTreeConfig config;
    config.numChildren = 4;
    config.percentagePriority = 0.1;
    config.depth = 3;
    config.circularLinks = false;
    config.doubleLinks = false;
    config.depthFirstAlloc = false;
    config.initialSeed = 1;

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
                config.depth = toData<std::size_t>(std::string(*(++argvc)));
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
        case optCircularLinks:
            config.circularLinks = true;
            break;
        case optDoubleLinks:
            config.doubleLinks = true;
            break;
        case optDepthFirst:
            config.depthFirstAlloc = true;
            break;
        case optSeed:
            ++i;
            if(i < argc){
                config.initialSeed = toData<std::size_t>(std::string(*(++argvc)));
                if(config.initialSeed == 0){
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

	adevs::Devs<t_event>* model = new PHOLDTree(config);

#ifdef USE_STAT
    #define USE_LISTENER
    adevs::EventListener<t_event>* listener = OutputCounter<t_event>();
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
	    allocateTree(static_cast<PHOLDTree*>(model), config, coreAmt);
		omp_set_num_threads(coreAmt);	//must manually set amount of OpenMP threads

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
