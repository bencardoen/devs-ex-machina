/*
 * tracers.h
 *
 *  Created on: Mar 30, 2015
 *      Author: Stijn Manhaeve - Devs Ex Machina
 */

#ifndef SRC_FORWARDDECLARE_TRACERS_H_
#define SRC_FORWARDDECLARE_TRACERS_H_

#include "verbosetracer.h"
#include "policies.h"

#ifdef VIRUSTRACER
#include "examples/virus/virustracer.h"
#endif

namespace n_tracers {
#ifndef VIRUSTRACER
/**
 * @brief Default set of tracers.
 * The default set of tracers uses only one Tracer (the Verbose tracer)
 * and sends its output to std::cout
 */
typedef Tracers<VerboseTracer<CoutWriter>> t_tracerset;

#else
typedef Tracers<VerboseTracer<FileWriter>, n_virus::VirusTracer> t_tracerset;

#endif

} /* namespace n_tracers */


#endif /* SRC_FORWARDDECLARE_TRACERS_H_ */
