/*
 * This file is part of the DEVS Ex Machina project.
 * Copyright 2014 - 2015 University of Antwerp 
 * https://www.uantwerpen.be/en/
 * Licensed under the EUPL V.1.1
 * A full copy of the license is in COPYING.txt, or can be found at 
 * https://joinup.ec.europa.eu/community/eupl/og_page/eupl 
 *      Author: Stijn Manhaeve
 */

#ifndef SRC_FORWARDDECLARE_TIMESTAMP_H_
#define SRC_FORWARDDECLARE_TIMESTAMP_H_

/** 
 * Define artificial load (sleep time in ms) for pdevs.
 */
#define PDEVS_LOAD 5

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
