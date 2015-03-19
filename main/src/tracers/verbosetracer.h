/*
 * verbosetracer.h
 *
 *  Created on: Mar 19, 2015
 *      Author: Stijn Manhaeve - Devs Ex Machina
 */

#ifndef SRC_TRACERS_VERBOSETRACER_H_
#define SRC_TRACERS_VERBOSETRACER_H_

#include "network/timestamp.h"
#include "tracers/tracemessage.h"
#include "tools/objectfactory.h"
#include "model/atomicmodel.h"

namespace n_tracers{

using namespace n_network;

template<typename OutputPolicy>
class VerboseTracer: public OutputPolicy
{
private:
	/**
	 * @brief Typedef for this class.
	 * In order to get the function pointers to functions defined in this class, use `&Derived::my_func`.
	 */
	typedef VerboseTracer<OutputPolicy> t_derived;

	t_timestamp m_prevTime;

public:
	/**
	 * @brief Performs the actual tracing. Once this function is called, there is no going back.
	 */
	void doTrace(t_timestamp time, std::ostringstream* ssr)
	{
		assert(ssr != nullptr);
		if(time > m_prevTime){
			OutputPolicy::print("\n__  Current Time: ", time, "____________________\n\n");
			m_prevTime = time;
		}
		OutputPolicy::print(ssr->str());	//print using the policy
		n_tools::takeBack(ssr);			//delete created ostringstream
	}
	/**
	 * @brief Traces state initialization of a model
	 * @param model The model that is initialized
	 * @param time The simulation time of initialization.
	 */
	void tracesInit(const t_atomicmodelptr& adevs, t_timestamp time)
	{
		assert(adevs != nullptr && "VerboseTracer::tracesInit argument cannot be a nullptr.");

		std::ostringstream* ssr = n_tools::createRawObject<std::ostringstream>();	//we don't need a raw object

		t_stateptr state = adevs->getState();
	        *ssr <<	"\n"
	        	"\tINITIAL CONDITIONS in model " << adevs->getName() << "\n"
	        	"\t\tInitial State: " << state->toString() << "\n"
	        << "\t\tNext scheduled internal transition at time " << adevs->timeAdvance().getTime() << "\n";

	        std::function<void()> fun = std::bind(&t_derived::doTrace, this, time, ssr);
		TraceMessage* message = n_tools::createRawObject<TraceMessage>(time, fun);
		//TODO deal with the trace message
	}

	/**
	 * @brief Traces internal state transition
	 * @param adevs The atomic model that just performed an internal transition
	 * @precondition The model pointer is not a nullptr
	 */
	void tracesInternal(const t_atomicmodelptr& adevs)
	{
		assert(adevs != nullptr && "VerboseTracer::tracesInternal argument cannot be a nullptr.");

		std::ostringstream* ssr = n_tools::createRawObject<std::ostringstream>();	//we don't need a raw object

		t_stateptr state = adevs->getState();
	        *ssr <<	"\n"
	        	"\tINTERNAL TRANSITION in model " << adevs->getName() << "\n"
	        	"\t\tNew State: " << state->toString() << "\n"
	        	"\t\tOutput Port Configuration:\n"
	        << "\t\t\t[Todo: actual port configuration]\n"
	        << "\t\tNext scheduled internal transition at time " << adevs->timeAdvance().getTime() << "\n";

	        t_timestamp time; //= adevs->getTimeLast();	//TODO get timestamp of the transition
		std::function<void()> fun = std::bind(&t_derived::doTrace, this, time, ssr);
		TraceMessage* message = n_tools::createRawObject<TraceMessage>(time, fun);
		//TODO deal with the trace message
	}
	/**
	 * @brief Traces external state transition
	 * @param model The model that just went through an external transition
	 */
	void tracesExternal(const t_atomicmodelptr& adevs)
	{
		assert(adevs != nullptr && "VerboseTracer::tracesExternal argument cannot be a nullptr.");

		std::ostringstream* ssr = n_tools::createRawObject<std::ostringstream>();	//we don't need a raw object

		t_stateptr state = adevs->getState();
	        *ssr <<	"\n"
	        	"\tEXTERNAL TRANSITION in model " << adevs->getName() << "\n"
	        	"\t\tNew State: " << state->toString() << "\n"
	        	"\t\tInput Port Configuration:\n"
	        << "\t\t\t[Todo: actual port configuration]\n"
	        << "\t\tNext scheduled internal transition at time " << adevs->timeAdvance().getTime() << "\n";

	        t_timestamp time; //= adevs->getTimeLast();	//TODO get timestamp of the transition
		std::function<void()> fun = std::bind(&t_derived::doTrace, this, time, ssr);
		TraceMessage* message = n_tools::createRawObject<TraceMessage>(time, fun);
		//TODO deal with the trace message
	}
	/**
	 * @brief Traces confluent state transition (simultaneous internal and external transition)
	 * @param model The model that just went through a confluent transition
	 */
	void tracesConfluent(const t_atomicmodelptr& adevs)
	{
		assert(adevs != nullptr && "VerboseTracer::tracesConfluent argument cannot be a nullptr.");

		std::ostringstream* ssr = n_tools::createRawObject<std::ostringstream>();	//we don't need a raw object

		t_stateptr state = adevs->getState();
	        *ssr <<	"\n"
	        	"\tCONFLUENT TRANSITION in model " << adevs->getName() << "\n"
	        	"\t\tInput Port Configuration:\n"
	        << "\t\t\t[Todo: actual port configuration]\n"
	        	"\t\tNew State: " << state->toString() << "\n"
	        	"\t\tOutput Port Configuration:\n"
	        << "\t\t\t[Todo: actual port configuration]\n"
	        << "\t\tNext scheduled internal transition at time " << adevs->timeAdvance().getTime() << "\n";

	        t_timestamp time; //= adevs->getTimeLast();	//TODO get timestamp of the transition
		std::function<void()> fun = std::bind(&t_derived::doTrace, this, time, ssr);
		TraceMessage* message = n_tools::createRawObject<TraceMessage>(time, fun);
		//TODO deal with the trace message
	}
};

} /* namespace n_tracers */


#endif /* SRC_TRACERS_VERBOSETRACER_H_ */
