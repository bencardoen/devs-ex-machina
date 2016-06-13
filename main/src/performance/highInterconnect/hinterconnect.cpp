/*
 * This file is part of the DEVS Ex Machina project.
 * Copyright 2014 - 2015 University of Antwerp
 * https://www.uantwerpen.be/en/
 * Licensed under the EUPL V.1.1
 * A full copy of the license is in COPYING.txt, or can be found at
 * https://joinup.ec.europa.eu/community/eupl/og_page/eupl
 *      Author: Stijn Manhaeve, Ben Cardoen
 */

#include "hinterconnect.h"
#include "tools/stringtools.h"
#include "tools/objectfactory.h"
#include <limits>


#ifdef FPTIME
#define T_0 0.0
#define T_1 0.01
#define T_50 0.5
#define T_75 0.75
#define T_100 1.0
#define T_125 1.25
#define T_STEP 0.01
#else
#define T_0 0u
#define T_1 1u
#define T_50 50u
#define T_75 75u
#define T_100 100u
#define T_125 125u
#define T_STEP 1u
#endif

namespace n_interconnect {

GeneratorState::GeneratorState(t_counter count):
	m_count(count)
{
}

Generator::Generator(const std::string& name, std::size_t initSeed, bool randta):
	AtomicModel(name), m_randomta(randta), m_out(addOutPort("output")), m_in(addInPort("input"))
{
    state().m_rand.seed(initSeed);
	adjustCounter();
}

template<typename T>
constexpr T roundTo(T val, T gran)
{
#ifdef __CYGWIN__
	return round(val/gran)*gran;
#else
	return std::round(val/gran)*gran;
#endif
}

void Generator::adjustCounter()
{
	GeneratorState& stat = state();
	if(!m_randomta){
		stat.m_count = T_100;
		return;
	}

#ifdef FPTIME
	std::uniform_real_distribution<t_counter> dist(T_1, T_100);
#else
	std::uniform_int_distribution<t_counter> dist(T_1, T_100);
#endif
	stat.m_count = dist(state().m_rand);
#ifdef FPTIME
	stat.m_count = roundTo(stat.m_count, T_STEP);
#endif
}

n_model::t_timestamp Generator::timeAdvance() const
{
	return n_model::t_timestamp(state().m_count);
}

void Generator::intTransition()
{
	adjustCounter();
}

void Generator::extTransition(const std::vector<n_network::t_msgptr>&)
{
	state().m_count -= getTimeElapsed().getTime();
}

void Generator::confTransition(const std::vector<n_network::t_msgptr>&)
{
	intTransition();
}

void Generator::output(std::vector<n_network::t_msgptr>& msgs) const
{
	m_out->createMessages(1, msgs);
}

n_network::t_timestamp Generator::lookAhead() const
{
	return n_network::t_timestamp(m_randomta? T_STEP : T_100);
}

Generator::~Generator()
{
	//nothing to do here
}

} /* namespace n_interconnect */
