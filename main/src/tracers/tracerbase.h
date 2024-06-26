/*
 * This file is part of the DEVS Ex Machina project.
 * Copyright 2014 - 2015 University of Antwerp
 * https://www.uantwerpen.be/en/
 * Licensed under the EUPL V.1.1
 * A full copy of the license is in COPYING.txt, or can be found at
 * https://joinup.ec.europa.eu/community/eupl/og_page/eupl
 *      Author: Stijn Manhaeve
 */

#ifndef SRC_TRACERS_TRACERBASE_H_
#define SRC_TRACERS_TRACERBASE_H_

#include <cstddef>	//std::size_t
#include <cassert>

namespace n_tracers {
/**
 * @brief Common base class for some tracers.
 *
 * This class takes care of creating messages, scheduling them and cleaning up the allocated memory.
 * The subclass is required to implement the following interface:
 * @code
	void doTrace(t_timestamp, std::ostringstream*);
	void tracesInitImpl(const t_atomicmodelptr&, t_timestamp, std::ostringstream*);
	void tracesInternalImpl(const t_atomicmodelptr&, std::ostringstream*);
	void tracesExternalImpl(const t_atomicmodelptr&, std::ostringstream*);
	void tracesConfluentImpl(const t_atomicmodelptr&, std::ostringstream*);
	void startTrace();
	void finishTrace();
 * @endcode
 * @see XmlTracer, @see JsonTracer, @see VerboseTracer for examples.
 */
template<typename T>
class TracerBase
{
private:
	typedef T Derived;
	typedef TracerBase<T> BaseType;

	void createMessage(const t_atomicmodelptr& adevs, std::size_t coreid, std::ostringstream* ssr)
	{
		t_timestamp time = adevs->getState()->m_timeLast; // get timestamp of the transition
		LOG_DEBUG("Tracer created a message at time", time);
		std::function<void()> fun = std::bind(&Derived::doTrace, static_cast<Derived*>(this), time, ssr);
		std::function<void()> takeback = std::bind(&BaseType::takeBack, this, ssr);
		t_tracemessageptr message = n_tools::createRawObject<TraceMessage>(time, fun, coreid, takeback);
		//deal with the message
		scheduleMessage(message);
	}

protected:
	TracerBase() = default;

public:

	/**
	 * @brief Cleans up any remaining data used for a particular trace message
	 */
	void takeBack(std::ostringstream* ssr)
	{
		delete ssr;
	}

	/**
	 * @brief Traces state initialization of a model
	 * @param model The model that is initialized
	 * @param time The simulation time of initialization.
	 */
	void tracesInit(const t_atomicmodelptr& adevs, t_timestamp time)
	{
		assert(adevs != nullptr && "VerboseTracer::tracesInit argument cannot be a nullptr.");

		std::ostringstream* ssr = n_tools::createRawObject<std::ostringstream>(); //we don't need a shared object

		static_cast<Derived*>(this)->tracesInitImpl(adevs, time, ssr);

		std::function<void()> fun = std::bind(&Derived::doTrace, static_cast<Derived*>(this), time, ssr);
		std::function<void()> takeback = std::bind(&BaseType::takeBack, this, ssr);
		t_tracemessageptr message = n_tools::createRawObject<TraceMessage>(time, fun, 0u, takeback);
		//deal with the message
		scheduleMessage(message);
	}

	/**
	 * @brief Traces internal state transition
	 * @param adevs The atomic model that just performed an internal transition
	 * @param coreid The ID of the core requesting the trace.
	 * @precondition The model pointer is not a nullptr
	 */
	void tracesInternal(const t_atomicmodelptr& adevs, std::size_t coreid)
	{
		assert(adevs != nullptr && "VerboseTracer::tracesInternal argument cannot be a nullptr.");

		std::ostringstream* ssr = n_tools::createRawObject<std::ostringstream>(); //we don't need a shared object

		static_cast<Derived*>(this)->tracesInternalImpl(adevs, ssr);

		createMessage(adevs, coreid, ssr);
	}
	/**
	 * @brief Traces external state transition
	 * @param model The model that just went through an external transition
	 * @param coreid The ID of the core requesting the trace.
	 */
	void tracesExternal(const t_atomicmodelptr& adevs, std::size_t coreid)
	{
		assert(adevs != nullptr && "VerboseTracer::tracesExternal argument cannot be a nullptr.");

		std::ostringstream* ssr = n_tools::createRawObject<std::ostringstream>(); //we don't need a shared object

		static_cast<Derived*>(this)->tracesExternalImpl(adevs, ssr);

		createMessage(adevs, coreid, ssr);
	}
	/**
	 * @brief Traces confluent state transition (simultaneous internal and external transition)
	 * @param model The model that just went through a confluent transition
	 * @param coreid The ID of the core requesting the trace.
	 */
	void tracesConfluent(const t_atomicmodelptr& adevs, std::size_t coreid)
	{
		assert(adevs != nullptr && "VerboseTracer::tracesConfluent argument cannot be a nullptr.");

		std::ostringstream* ssr = n_tools::createRawObject<std::ostringstream>(); //we don't need a shared object

		static_cast<Derived*>(this)->tracesConfluentImpl(adevs, ssr);

		createMessage(adevs, coreid, ssr);
	}
};

} /* namespace n_tracers */

#endif /* SRC_TRACERS_TRACERBASE_H_ */
