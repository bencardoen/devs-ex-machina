/*
 * jsontracer.h
 *
 *  Created on: Apr 27, 2015
 *      Author: Stijn Manhaeve - Devs Ex Machina
 */

#ifndef SRC_TRACERS_JSONTRACER_H_
#define SRC_TRACERS_JSONTRACER_H_

#include "timestamp.h"
#include "tracemessage.h"
#include "objectfactory.h"
#include "atomicmodel.h"
#include "tracerbase.h"

namespace n_tracers{

using namespace n_network;

/**
 * @brief Tracer that will generate xml output.
 * @tparam OutputPolicy A policy that dictates what should happen with the output
 * @note The structure of the trace output is based on the xml structure.
 * 	 If a standardized structure already exists, please either send us a mail or fix this issue here
 */
template<typename OutputPolicy>
class JsonTracer: public OutputPolicy, public TracerBase<JsonTracer<OutputPolicy>>
{
private:
	char m_comma = ' ';
	/**
	 * @brief Typedef for this class.
	 * In order to get the function pointers to functions defined in this class, use `&Derived::my_func`.
	 */
	typedef JsonTracer<OutputPolicy> t_derived;

	inline void printIncoming(const t_atomicmodelptr& adevs, std::ostringstream* ssr){
		const std::map<std::string, t_portptr>& ports = adevs->getIPorts();
		char comma1 = ' ';
		for (const std::map<std::string, t_portptr>::value_type& item : ports) {
			*ssr << comma1 << "{ \"name\":\"" << item.first << "\", \"category\":\"I\",\n"
				"\"messages\":[";
			const std::vector<n_network::t_msgptr>& messages = item.second->getReceivedMessages();
			char comma2 = ' ';
			for (const n_network::t_msgptr& message : messages){
				*ssr << comma2 << "{\"message\": " << message->getPayload() << "}";
				comma2 = ',';
			}
			*ssr << "]}";
			comma1 = ',';
		}
	}

	inline void printOutgoing(const t_atomicmodelptr& adevs, std::ostringstream* ssr){
		const std::map<std::string, t_portptr>& ports = adevs->getOPorts();
		char comma1 = ' ';
		for (const std::map<std::string, t_portptr>::value_type& item : ports) {
			*ssr << comma1 << "{ \"name\":\"" << item.first << "\", \"category\":\"O\",\n"
				"\"messages\":[";
			const std::vector<n_network::t_msgptr>& messages = item.second->getReceivedMessages();
			char comma2 = ' ';
			for (const n_network::t_msgptr& message : messages){
				*ssr << comma2 << "{\"message\": " << message->getPayload() << "}";
				comma2 = ',';
			}
			*ssr << "]}";
			comma1 = ',';
		}
	}

public:
	JsonTracer() = default;
	/**
	 * @brief Performs the actual tracing. Once this function is called, there is no going back.
	 */
	void doTrace(t_timestamp, std::ostringstream* ssr)
	{
		assert(ssr != nullptr);
		OutputPolicy::print(m_comma, ssr->str());	//print using the policy
		m_comma = ',';
	}
	/**
	 * @brief Traces state initialization of a model
	 * @param model The model that is initialized
	 * @param time The simulation time of initialization.
	 */
	inline void tracesInitImpl(const t_atomicmodelptr& adevs, t_timestamp nextT, std::ostringstream* ssr)
	{
		t_stateptr state = adevs->getState();
		*ssr << "{\n"
			"\"model\":\"" << adevs->getName() << "\",\n"
			"\"time\":";
		if(nextT == t_timestamp::infinity())
			*ssr << '"' << nextT << '"';
		else *ssr << nextT.getTime();
		*ssr << ",\n"
			"\"kind\":\"" "EX" "\",\n"
			"\"state\":{\"object\":" << state->toJSON() << ", \"text\":\"" << state->toString() << "\"}\n"
			"}\n";
	}

	/**
	 * @brief Traces internal state transition
	 * @param adevs The atomic model that just performed an internal transition
	 * @precondition The model pointer is not a nullptr
	 */
	inline void tracesInternalImpl(const t_atomicmodelptr& adevs, std::ostringstream* ssr)
	{
		t_stateptr state = adevs->getState();
		t_timestamp nextT =  adevs->getTimeNext();
		*ssr << "{\n"
			"\"model\":\"" << adevs->getName() << "\",\n"
			"\"time\":";
		if(nextT == t_timestamp::infinity())
			*ssr << '"' << nextT << '"';
		else *ssr << nextT.getTime();
		*ssr << ",\n"
			"\"kind\":\"" "IN" "\",\n"
			"\"ports\":[";
		printOutgoing(adevs, ssr);
		*ssr << "],\n\"state\":{\"object\":" << state->toJSON() << ", \"text\":\"" << state->toString() << "\"}\n"
			"}\n";
	}
	/**
	 * @brief Traces external state transition
	 * @param model The model that just went through an external transition
	 */
	inline void tracesExternalImpl(const t_atomicmodelptr& adevs, std::ostringstream* ssr)
	{
		t_stateptr state = adevs->getState();
		t_timestamp nextT =  adevs->getTimeNext();
		*ssr << "{\n"
			"\"model\":\"" << adevs->getName() << "\",\n"
			"\"time\":";
		if(nextT == t_timestamp::infinity())
			*ssr << '"' << nextT << '"';
		else *ssr << nextT.getTime();
		*ssr << ",\n"
			"\"kind\":\"" "EX" "\",\n"
			"\"ports\":[";
		printIncoming(adevs, ssr);
		*ssr << "],\n\"state\":{\"object\":" << state->toJSON() << ", \"text\":\"" << state->toString() << "\"}\n"
			"}\n";
	}
	/**
	 * @brief Traces confluent state transition (simultaneous internal and external transition)
	 * @param model The model that just went through a confluent transition
	 */
	inline void tracesConfluentImpl(const t_atomicmodelptr& adevs, std::ostringstream* ssr)
	{
		tracesExternalImpl(adevs, ssr);
		*ssr << ',';
		tracesInternalImpl(adevs, ssr);
	}

	/**
	 * @brief Traces the  start of the output
	 * Certain tracers can use this to generate a header or similar
	 */
	inline void startTrace()
	{
		OutputPolicy::print("{\"events\":[\n");
	}

	/**
	 * @brief Finishes the trace output
	 * Certain tracers can use this to generate a footer or similar
	 */
	inline void finishTrace()
	{
		OutputPolicy::print("\n]}");
	}
};

} /* namespace n_tracers */


#endif /* SRC_TRACERS_JSONTRACER_H_ */
