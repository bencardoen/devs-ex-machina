/*
 * celltracer.h
 *
 *  Created on: May 1, 2015
 *      Author: Stijn Manhaeve - Devs Ex Machina
 */

#ifndef SRC_TRACERS_CELLTRACER_H_
#define SRC_TRACERS_CELLTRACER_H_

#include <cellmodel.h>
#include "timestamp.h"
#include "tracemessage.h"
#include "objectfactory.h"
#include "atomicmodel.h"
#include "policies.h"
#include <array>

namespace n_tracers{

using namespace n_network;

/**
 * @brief Tracer that will generate cell output.
 * @tparam OutputPolicy A policy that dictates what should happen with the output
 *
 * This tracer requires the models to be a subclass of the CellAtomicModel class.
 * If a model does not derive from this type, it is ignored.
 */
template<typename OutputPolicy, std::size_t XSize, std::size_t YSize>
class CellTracer: public OutputPolicy
{
private:
	/**
	 * @brief Typedef for this class.
	 * In order to get the function pointers to functions defined in this class, use `&Derived::my_func`.
	 */
	typedef CellTracer<OutputPolicy, XSize, YSize> t_derived;
	typedef std::array<std::array<std::string, YSize>, XSize> t_cellgrid;
	//keeps track of all the stuff in the arrays

	t_cellgrid m_cells;

	inline void traceCall(const n_model::t_atomicmodelptr& adevs, std::size_t coreid){
		t_timestamp time = adevs->getState()->m_timeLast; // get timestamp of the transition
		traceCall(adevs, coreid, time);
	}

	inline void traceCall(const n_model::t_atomicmodelptr& adevs, std::size_t coreid, t_timestamp time){
		LOG_DEBUG("CellTracer created a message at time", time);

		n_model::t_cellmodelptr celldevs = std::dynamic_pointer_cast<n_model::CellAtomicModel>(adevs);
		if(!celldevs) return;	//don't celltrace models that don't have the correct type of state

		std::function<void()> fun = std::bind(&t_derived::doTrace, this, time, celldevs->getPoint(), celldevs->getState()->toCell());
		t_tracemessageptr message = n_tools::createRawObject<TraceMessage>(time, fun, coreid);
		//deal with the message
		scheduleMessage(message);
	}

	//If we write to multiple files
	//  do NOT print a header for this entry
	//otherwise
	//  do print a simple header denoting the current time
	template<class Policy = OutputPolicy>
	inline typename std::enable_if<std::is_same<Policy, MultiFileWriter>::value>::type
	actualTrace(t_timestamp){
		OutputPolicy::startNewFile();
		for(std::size_t x = 0; x < XSize; ++x) {
			for(std::size_t y = 0; y < XSize; ++y) {
				OutputPolicy::print(m_cells[x][y], ' ');
			}
			OutputPolicy::print('\n');
		}
	}

	template<class Policy = OutputPolicy>
	inline typename std::enable_if<!std::is_same<Policy, MultiFileWriter>::value>::type
	actualTrace(t_timestamp time){
		OutputPolicy::print("=== At time ", time.getTime(), " ===\n");
		for(std::size_t x = 0; x < XSize; ++x) {
			for(std::size_t y = 0; y < XSize; ++y) {
				OutputPolicy::print(m_cells[x][y], ' ');
			}
			OutputPolicy::print('\n');
		}
	}
	t_timestamp m_prevTime;

public:
	/**
	 * @brief Constructs a new CellTracer object.
	 * @note Depending on which OutputPolicy is used, this tracer must be initialized before it can be used. See the documentation of the policy itself.
	 */
	CellTracer(): m_prevTime(0, std::numeric_limits<t_timestamp::t_causal>::max())
	{
	}
	/**
	 * @brief Performs the actual tracing. Once this function is called, there is no going back.
	 */
	void doTrace(t_timestamp time, t_point pt, std::string str)
	{
		if (time.getTime() > m_prevTime.getTime()) {// || m_prevTime == t_timestamp(0, std::numeric_limits<t_timestamp::t_causal>::max())) {
			actualTrace(time);
			m_prevTime = time;
		}
		//save state ptr in the point
		m_cells.at(pt.first).at(pt.second) = str;
	}
	/**
	 * @brief Traces state initialization of a model
	 * @param model The model that is initialized
	 * @param time The simulation time of initialization.
	 * @note No trace will be made if the model does not have a CellState
	 * @see n_model::CellState
	 */
	inline void tracesInit(const t_atomicmodelptr& adevs, t_timestamp time)
	{
		traceCall(adevs, 0, time);
	}

	/**
	 * @brief Traces internal state transition
	 * @param adevs The atomic model that just performed an internal transition
	 * @precondition The model pointer is not a nullptr
	 * @note No trace will be made if the model does not have a CellState
	 * @see n_model::CellState
	 */
	inline void tracesInternal(const t_atomicmodelptr& adevs, std::size_t coreid)
	{
		traceCall(adevs, coreid);
	}
	/**
	 * @brief Traces external state transition
	 * @param model The model that just went through an external transition
	 * @note No trace will be made if the model does not have a CellState
	 * @see n_model::CellState
	 */
	inline void tracesExternal(const t_atomicmodelptr& adevs, std::size_t coreid)
	{
		traceCall(adevs, coreid);
	}
	/**
	 * @brief Traces confluent state transition (simultaneous internal and external transition)
	 * @param model The model that just went through a confluent transition
	 * @note No trace will be made if the model does not have a CellState
	 * @see n_model::CellState
	 */
	inline void tracesConfluent(const t_atomicmodelptr& adevs, std::size_t coreid)
	{
		traceCall(adevs, coreid);
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
