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
	const std::size_t m_id;
	static std::size_t m_lastid;

protected:
	TracerBase()
		: m_id(m_lastid++)
	{
	}

public:
	/**
	 * @return the ID of this tracer.
	 * Each tracer has a unique ID that can be used to order
	 * TracerMessages of different tracers for the same event and stuff like that.
	 */
	std::size_t getID() const
	{
		return m_id;
	}
};

} /* namespace n_tracers */

#endif /* SRC_TRACERS_TRACERBASE_H_ */
