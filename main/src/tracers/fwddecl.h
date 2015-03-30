/*
 * fwddecl.h
 *
 *  Created on: Mar 30, 2015
 *      Author: Stijn Manhaeve - Devs Ex Machina
 */

#ifndef SRC_TRACERS_FWDDECL_H_
#define SRC_TRACERS_FWDDECL_H_

#include "verbosetracer.h"
#include "policies.h"

namespace n_tracers {
//template<typename ... TracerElems>
//class Tracers;

/**
 * @brief Default set of tracers.
 * The default set of tracers uses only one Tracer (the Verbose tracer)
 * and sends its output to std::cout
 */
typedef Tracers<VerboseTracer<CoutWriter>> t_tracerset;

/**
 * @brief Default pointer type to the default set of tracers
 * @see t_tracerset
 */
typedef std::shared_ptr<t_tracerset> t_tracersetptr;

} /* namespace n_tracers */


#endif /* SRC_TRACERS_FWDDECL_H_ */
