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
 * @brief Default type for timestamps.
 * The default implementation for time uses std::size_t
 * for both its time and causality component.
 */
typedef Time<std::size_t, std::size_t> t_timestamp;

} /* namespace n_network */

#endif /* SRC_FORWARDDECLARE_TIMESTAMP_H_ */
