/*
 * tracers.h
 *
 *  Created on: Mar 30, 2015
 *      Author: Stijn Manhaeve - Devs Ex Machina
 */

#ifndef SRC_FORWARDDECLARE_TRACERS_H_
#define SRC_FORWARDDECLARE_TRACERS_H_

#include "tracers/verbosetracer.h"
#include "tracers/policies.h"

#ifdef VIRUSTRACER
#include "examples/virus/virustracer.h"
#endif

namespace n_tracers {
#ifdef VIRUSTRACER
typedef Tracers<VerboseTracer<FileWriter>, n_virus::VirusTracer> t_tracerset;

#else
#ifdef BENCHMARK
/**
 * @brief Benchmark tracer
 * The benchmarks do not generate tracing output. Only the simulation core is benchmarked to keep things fair.
 */
typedef Tracers<> t_tracerset;
#else
/**
 * @brief Default set of tracers.
 * The default set of tracers uses only one Tracer (the Verbose tracer)
 * and sends its output to std::cout
 */
typedef Tracers<VerboseTracer<CoutWriter>> t_tracerset;

#endif	//#ifdef BENCHMARK
#endif	//#ifdef VIRUSTRACER

} /* namespace n_tracers */


#endif /* SRC_FORWARDDECLARE_TRACERS_H_ */
