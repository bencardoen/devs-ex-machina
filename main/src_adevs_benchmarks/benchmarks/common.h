/*
 * This file is part of the DEVS Ex Machina project.
 * Copyright 2014 - 2015 University of Antwerp
 * https://www.uantwerpen.be/en/
 * Licensed under the EUPL V.1.1
 * A full copy of the license is in COPYING.txt, or can be found at
 * https://joinup.ec.europa.eu/community/eupl/og_page/eupl
 *      Author: Stijn Manhaeve, Ben Cardoen
 */

#ifndef SRC_ADEVS_BENCHMARKS_BENCHMARKS_COMMON_H_
#define SRC_ADEVS_BENCHMARKS_BENCHMARKS_COMMON_H_

#include <adevs.h>
#include <tools/statistic.h>
#include <fstream>

template<typename t_event>
class OutputCounter: public adevs::EventListener<t_event>
{
private:
    n_tools::t_uintstat m_outEventCounter;  //counts the number of output events
    std::string m_ofile;
public:
    OutputCounter(std::string out = "stats.txt"): m_outEventCounter("output-events", ""), m_ofile(out){}
    virtual void outputEvent(adevs::Event<t_event,double>, double)
    {
        #pragma omp critical (OutputCounterCriticalSection)
        {
            ++m_outEventCounter;
        }
    }
    virtual void stateChange(adevs::Atomic<t_event>*, double)
    { /* noop */ }

    virtual ~OutputCounter()
    {
        // destructor of the counter will print out the data
        std::ofstream out(m_ofile);
        out << m_outEventCounter;
    }
};

extern double T_INF;

template<typename EventType, typename ToName, typename t_eventTime = double>
class Listener: public adevs::EventListener<EventType>
{
    double m_lastTime;
public:
    Listener(): m_lastTime(0.0){
        std::cout << "note: this Listener is not threadsafe!\n";
    }
    virtual void outputEvent(adevs::Event<EventType, t_eventTime> x, double t){
        if(t > m_lastTime) {
            std::cout << "\n__  Current Time: " << t << "____________________\n\n\n";
            m_lastTime = t;
        }
        std::cout << "output by: (" << ToName::eval(x.model) << ")\n";
    }
    virtual void stateChange(adevs::Atomic<EventType>* model, double t){
        if(t > m_lastTime) {
            std::cout << "\n__  Current Time: " << t << "____________________\n\n\n";
            m_lastTime = t;
        }
        std::cout << "stateChange by: (" << ToName::eval(model) << ")\n";
        if(model->ta() == T_INF)
            std::cout << "\t\tNext scheduled internal transition at time inf\n";
        else
            std::cout << "\t\tNext scheduled internal transition at time " << t + model->ta() << '\n';
    }

    virtual ~Listener(){}
};


#endif /* SRC_ADEVS_BENCHMARKS_BENCHMARKS_COMMON_H_ */
