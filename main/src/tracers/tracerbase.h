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
class TracerBase
{
private:

protected:
	TracerBase()
	{
	}

public:
};

} /* namespace n_tracers */

#endif /* SRC_TRACERS_TRACERBASE_H_ */
