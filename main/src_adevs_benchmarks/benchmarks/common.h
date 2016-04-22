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
        ++m_outEventCounter;
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



#endif /* SRC_ADEVS_BENCHMARKS_BENCHMARKS_COMMON_H_ */
