/*
 * tracerbase.h
 *
 *  Created on: Mar 28, 2015
 *      Author: Stijn Manhaeve - Devs Ex Machina
 */

#ifndef SRC_TRACERS_TRACERBASE_H_
#define SRC_TRACERS_TRACERBASE_H_

#include <cstddef>	//std::size_t
#include <cassert>

namespace n_tracers {
/**
 * @brief Common base class for all tracers.
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
	void takeBack(std::ostringstream* ssr){
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

		std::ostringstream* ssr = n_tools::createRawObject<std::ostringstream>();//we don't need a shared object

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
	 * @precondition The model pointer is not a nullptr
	 */
	void tracesInternal(const t_atomicmodelptr& adevs, std::size_t coreid)
	{
		assert(adevs != nullptr && "VerboseTracer::tracesInternal argument cannot be a nullptr.");

		std::ostringstream* ssr = n_tools::createRawObject<std::ostringstream>();//we don't need a shared object

		static_cast<Derived*>(this)->tracesInternalImpl(adevs, ssr);

		createMessage(adevs, coreid, ssr);
	}
	/**
	 * @brief Traces external state transition
	 * @param model The model that just went through an external transition
	 */
	void tracesExternal(const t_atomicmodelptr& adevs, std::size_t coreid)
	{
		assert(adevs != nullptr && "VerboseTracer::tracesExternal argument cannot be a nullptr.");

		std::ostringstream* ssr = n_tools::createRawObject<std::ostringstream>();//we don't need a shared object

		static_cast<Derived*>(this)->tracesExternalImpl(adevs, ssr);

		createMessage(adevs, coreid, ssr);
	}
	/**
	 * @brief Traces confluent state transition (simultaneous internal and external transition)
	 * @param model The model that just went through a confluent transition
	 */
	void tracesConfluent(const t_atomicmodelptr& adevs, std::size_t coreid)
	{
		assert(adevs != nullptr && "VerboseTracer::tracesConfluent argument cannot be a nullptr.");

		std::ostringstream* ssr = n_tools::createRawObject<std::ostringstream>();//we don't need a shared object

		static_cast<Derived*>(this)->tracesConfluentImpl(adevs, ssr);

		createMessage(adevs, coreid, ssr);
	}
};

} /* namespace n_tracers */

#endif /* SRC_TRACERS_TRACERBASE_H_ */
