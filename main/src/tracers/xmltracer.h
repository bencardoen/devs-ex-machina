/*
 * xmltracer.h
 *
 *  Created on: Apr 26, 2015
 *      Author: Stijn Manhaeve - Devs Ex Machina
 */

#ifndef SRC_TRACERS_XMLTRACER_H_
#define SRC_TRACERS_XMLTRACER_H_

#include "network/timestamp.h"
#include "tracers/tracemessage.h"
#include "tools/objectfactory.h"
#include "model/atomicmodel.h"
#include "tracers/tracerbase.h"

namespace n_tracers{

using namespace n_network;

/**
 * @brief Tracer that will generate xml output.
 * @tparam OutputPolicy A policy that dictates what should happen with the output
 *
 * The generated xml will have the structure expected by the DEVS Trace Plotter by Hongyan (Bill) Song
 * @note The MultifilePolicy is not supported by this tracer.
 */
template<typename OutputPolicy>
class XmlTracer: public OutputPolicy, public TracerBase<XmlTracer<OutputPolicy>>
{
private:
	/**
	 * @brief Typedef for this class.
	 * In order to get the function pointers to functions defined in this class, use `&Derived::my_func`.
	 */
	typedef XmlTracer<OutputPolicy> t_derived;

	inline void printIncoming(const t_atomicmodelptr& adevs, std::ostringstream* ssr)
	{
		const std::map<std::string, t_portptr>& ports = adevs->getIPorts();
		for (const std::map<std::string, t_portptr>::value_type& item : ports) {
			*ssr << "<port name=\"" << item.first << "\" category=\"I\">\n";
			const std::vector<n_network::t_msgptr>& messages = item.second->getReceivedMessages();
			for (const n_network::t_msgptr& message : messages)
				*ssr << "<message>" << message->getPayload() << "</message>\n";
			*ssr << "</port>\n";
		}
	}

	inline void printOutgoing(const t_atomicmodelptr& adevs, std::ostringstream* ssr)
	{
		const std::map<std::string, t_portptr>& ports = adevs->getOPorts();
		for (const std::map<std::string, t_portptr>::value_type& item : ports) {
			*ssr << "<port name=\"" << item.first << "\" category=\"O\">\n";
			const std::vector<n_network::t_msgptr>& messages = item.second->getSentMessages();
			for (const n_network::t_msgptr& message : messages)
				*ssr << "<message>" << message->getPayload() << "</message>\n";
			*ssr << "</port>\n";
		}
	}

public:
	/**
	 * @brief Constructs a new XmlTracer object.
	 * @note Depending on which OutputPolicy is used, this tracer must be initialized before it can be used. See the documentation of the policy itself.
	 */
	XmlTracer() = default;
	/**
	 * @brief Performs the actual tracing. Once this function is called, there is no going back.
	 */
	void doTrace(t_timestamp, std::ostringstream* ssr)
	{
		assert(ssr != nullptr);
		OutputPolicy::print(ssr->str());	//print using the policy
	}
	/**
	 * @brief Traces state initialization of a model
	 * @param model The model that is initialized
	 * @param time The simulation time of initialization.
	 */
	inline void tracesInitImpl(const t_atomicmodelptr& adevs, t_timestamp nextT, std::ostringstream* ssr)
	{
		t_stateptr state = adevs->getState();
		*ssr << "<event>\n"
			"<model>" << adevs->getName() << "</model>\n"
			"<time>";
		if(isInfinity(nextT))
			*ssr << nextT;
		else *ssr << nextT.getTime();
		*ssr << "</time>\n"
			"<kind>" "EX" "</kind>\n"
			"<state>" << state->toXML() << "<![CDATA[" << state->toString() << "]]>\n</state>\n"
			"</event>\n";
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
		*ssr << "<event>\n"
			"<model>" << adevs->getName() << "</model>\n"
			"<time>";
		if(isInfinity(nextT))
			*ssr << nextT;
		else *ssr << nextT.getTime();
		*ssr << "</time>\n"
			"<kind>" "IN" "</kind>\n";
		printOutgoing(adevs, ssr);
		*ssr << "<state>" << state->toXML() << "<![CDATA[" << state->toString() << "]]>\n</state>\n"
			"</event>\n";
	}
	/**
	 * @brief Traces external state transition
	 * @param model The model that just went through an external transition
	 */
	inline void tracesExternalImpl(const t_atomicmodelptr& adevs, std::ostringstream* ssr)
	{
		t_stateptr state = adevs->getState();
		t_timestamp nextT =  adevs->getTimeNext();
		*ssr << "<event>\n"
			"<model>" << adevs->getName() << "</model>\n"
			"<time>";
		if(isInfinity(nextT))
			*ssr << nextT;
		else *ssr << nextT.getTime();
		*ssr << "</time>\n"
			"<kind>" "EX" "</kind>\n";
		printIncoming(adevs, ssr);
		*ssr << "<state>" << state->toXML() << "<![CDATA[" << state->toString() << "]]>\n</state>\n"
			"</event>\n";
	}
	/**
	 * @brief Traces confluent state transition (simultaneous internal and external transition)
	 * @param model The model that just went through a confluent transition
	 */
	inline void tracesConfluentImpl(const t_atomicmodelptr& adevs, std::ostringstream* ssr)
	{
		tracesExternalImpl(adevs, ssr);
		tracesInternalImpl(adevs, ssr);
	}

	/**
	 * @brief Traces the  start of the output
	 * Certain tracers can use this to generate a header or similar
	 */
	inline void startTrace()
	{
		OutputPolicy::print("<?xml version=\"1.0\"?>\n<trace>\n");
	}

	/**
	 * @brief Finishes the trace output
	 * Certain tracers can use this to generate a footer or similar
	 */
	inline void finishTrace()
	{
		OutputPolicy::print("</trace>");
	}
};

} /* namespace n_tracers */


#endif /* SRC_TRACERS_XMLTRACER_H_ */
