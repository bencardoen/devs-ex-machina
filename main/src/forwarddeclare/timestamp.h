/*
 * timestamp.h
 *
 *  Created on: Apr 23, 2015
 *      Author: Ben, Stijn - Devs Ex Machina
 */

#ifndef SRC_FORWARDDECLARE_TIMESTAMP_H_
#define SRC_FORWARDDECLARE_TIMESTAMP_H_


namespace n_network {
/**
 * Typedefs for time in simulator. Unless FPTIME is defined, time is
 * represented by an unsigned integer (usually 64 bit).
 */
#ifdef FPTIME
typedef Time<double, std::size_t> t_timestamp;
#else
typedef Time<std::size_t, std::size_t> t_timestamp;
#endif

#ifndef EPSILON_FPTIME
#define EPSILON_FPTIME 2.0e-12
#endif

} /* namespace n_network */

#endif /* SRC_FORWARDDECLARE_TIMESTAMP_H_ */
