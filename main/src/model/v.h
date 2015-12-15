/*
 * This file is part of the DEVS Ex Machina project.
 * Copyright 2014 - 2015 University of Antwerp
 * https://www.uantwerpen.be/en/
 * Licensed under the EUPL V.1.1
 * A full copy of the license is in COPYING.txt, or can be found at
 * https://joinup.ec.europa.eu/community/eupl/og_page/eupl
 *      Author: Ben Cardoen, Tim Tuijn
 */

#ifndef SRC_MODEL_V_H_
#define SRC_MODEL_V_H_

#include <vector>

namespace n_model {

/**
 * Thread-shared messagecount vector.
 * Used in Mattern's algorithm, together with Count vector stores sent/recd messages to
 * check for transient messages.
 */
class V
{
private:
	std::vector<int> m_vec;
public:
	std::vector<int>&
	getVector()
	{
		return m_vec;
	}
	V(std::size_t cores)
		: m_vec(cores)
	{
		;
	}
	virtual ~V()
	{
		;
	}
};

} /* namespace n_model */

#endif /* SRC_MODEL_V_H_ */
