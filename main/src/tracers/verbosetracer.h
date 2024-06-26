/*
 * This file is part of the DEVS Ex Machina project.
 * Copyright 2014 - 2015 University of Antwerp
 * https://www.uantwerpen.be/en/
 * Licensed under the EUPL V.1.1
 * A full copy of the license is in COPYING.txt, or can be found at
 * https://joinup.ec.europa.eu/community/eupl/og_page/eupl
 *      Author: Stijn Manhaeve, Tim Tuijn
 */

#ifndef SRC_TRACERS_VERBOSETRACER_H_
#define SRC_TRACERS_VERBOSETRACER_H_

#include "network/timestamp.h"
#include "tracers/tracemessage.h"
#include "tools/objectfactory.h"
#include "model/atomicmodel.h"
#include "tracers/tracerbase.h"

namespace n_tracers{

using namespace n_network;

/**
 * @brief Tracer that will generate verbose output.
 * @tparam OutputPolicy A policy that dictates what should happen with the output
 * @note The MultifilePolicy is not supported by this tracer.
 */
template<typename OutputPolicy>
class VerboseTracer: public OutputPolicy, public TracerBase<VerboseTracer<OutputPolicy>>
{
private:
	/**
	 * @brief Typedef for this class.
	 * In order to get the function pointers to functions defined in this class, use `&Derived::my_func`.
	 */
	typedef VerboseTracer<OutputPolicy> t_derived;

	t_timestamp m_prevTime;

	inline void printIncoming(const t_atomicmodelptr& adevs, std::ostringstream* ssr)
	{
		const std::vector<t_portptr>& ports = adevs->getIPorts();
		for (const t_portptr& item : ports) {
			*ssr << "\t\t\tport <" << item->getName() << ">:\n";
			const std::vector<n_network::t_msgptr>& messages = item->getReceivedMessages();
			for (const n_network::t_msgptr& message : messages)
				*ssr << "\t\t\t\t" << message->getPayload() << '\n';	// message->toString()?
		}
	}

	inline void printOutgoing(const t_atomicmodelptr& adevs, std::ostringstream* ssr)
	{
		const std::vector<t_portptr>& ports = adevs->getOPorts();
		for (const t_portptr& item : ports) {
			*ssr << "\t\t\tport <" << item->getName() << ">:\n";
			const std::vector<n_network::t_msgptr>& messages = item->getSentMessages();
			for (const n_network::t_msgptr& message : messages)
				*ssr << "\t\t\t\t" << message->getPayload() << '\n';	// message->toString()?
		}
	}

public:
	/**
	 * @brief Constructs a new VerboseTracer object.
	 * @note Depending on which OutputPolicy is used, this tracer must be initialized before it can be used. See the documentation of the policy itself.
	 */
	VerboseTracer(): m_prevTime(0, std::numeric_limits<t_timestamp::t_causal>::max())
	{
	}
	/**
	 * @brief Performs the actual tracing. Once this function is called, there is no going back.
	 */
	void doTrace(t_timestamp time, std::ostringstream* ssr)
	{
		assert(ssr != nullptr);
		if (time.getTime() > m_prevTime.getTime() || m_prevTime == t_timestamp(0, std::numeric_limits<t_timestamp::t_causal>::max())) {
			OutputPolicy::print("\n__  Current Time: ", time.getTime(), "____________________\n\n");//do not print causality
			m_prevTime = time;
		}
		OutputPolicy::print(ssr->str());	//print using the policy
	}
	/**
	 * @brief Traces state initialization of a model
	 * @param model The model that is initialized
	 * @param time The simulation time of initialization.
	 */
	inline void tracesInitImpl(const t_atomicmodelptr& adevs, t_timestamp, std::ostringstream* ssr)
	{
		t_stateptr state = adevs->getState();
		*ssr << "\n"
			"\tINITIAL CONDITIONS in model " << adevs->getName() << "\n"
			"\t\tInitial State: " << state->toString() << "\n"
		        "\t\tNext scheduled internal transition at time ";
		t_timestamp nextT =  adevs->getTimeNext();
		if(isInfinity(nextT))
			*ssr << nextT;
		else *ssr << nextT.getTime();
		*ssr << '\n';
	}

	/**
	 * @brief Traces internal state transition
	 * @param adevs The atomic model that just performed an internal transition
	 * @precondition The model pointer is not a nullptr
	 */
	inline void tracesInternalImpl(const t_atomicmodelptr& adevs, std::ostringstream* ssr)
	{
		t_stateptr state = adevs->getState();
		*ssr << "\n"
			"\tINTERNAL TRANSITION in model " << adevs->getName() << "\n"
			"\t\tNew State: " << state->toString() << "\n"
			"\t\tOutput Port Configuration:\n";

		printOutgoing(adevs, ssr);

	        *ssr << "\t\tNext scheduled internal transition at time ";
		t_timestamp nextT =  adevs->getTimeNext();
		if(isInfinity(nextT))
			*ssr << nextT;
		else *ssr << nextT.getTime();
		*ssr << '\n';
	}
	/**
	 * @brief Traces external state transition
	 * @param model The model that just went through an external transition
	 */
	inline void tracesExternalImpl(const t_atomicmodelptr& adevs, std::ostringstream* ssr)
	{
		t_stateptr state = adevs->getState();
		*ssr << "\n"
			"\tEXTERNAL TRANSITION in model " << adevs->getName() << "\n"
			"\t\tNew State: " << state->toString() << "\n"
			"\t\tInput Port Configuration:\n";

		printIncoming(adevs, ssr);

	        *ssr << "\t\tNext scheduled internal transition at time ";
		t_timestamp nextT =  adevs->getTimeNext();
		if(isInfinity(nextT))
			*ssr << nextT;
		else *ssr << nextT.getTime();
		*ssr << '\n';
	}
	/**
	 * @brief Traces confluent state transition (simultaneous internal and external transition)
	 * @param model The model that just went through a confluent transition
	 */
	inline void tracesConfluentImpl(const t_atomicmodelptr& adevs, std::ostringstream* ssr)
	{
		t_stateptr state = adevs->getState();
		*ssr << "\n"
			"\tCONFLUENT TRANSITION in model " << adevs->getName() << "\n"
			"\t\tInput Port Configuration:\n";

		printIncoming(adevs, ssr);

		*ssr << "\t\tNew State: " << state->toString() << "\n"
			"\t\tOutput Port Configuration:\n";

		printOutgoing(adevs, ssr);

	        *ssr << "\t\tNext scheduled internal transition at time ";
		t_timestamp nextT =  adevs->getTimeNext();
		if(isInfinity(nextT))
			*ssr << nextT;
		else *ssr << nextT.getTime();
		*ssr << '\n';
	}

	/**
	 * @brief Traces the  start of the output
	 * Certain tracers can use this to generate a header or similar
	 */
	inline void startTrace()
	{
	}

	/**
	 * @brief Finishes the trace output
	 * Certain tracers can use this to generate a footer or similar
	 */
	inline void finishTrace()
	{
	}
};

} /* namespace n_tracers */


#endif /* SRC_TRACERS_VERBOSETRACER_H_ */
