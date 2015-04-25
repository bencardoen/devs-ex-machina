/*
 * v.h
 *  Created on: 10 Apr 2015
 *      Author: Ben Cardoen
 */

#ifndef SRC_MODEL_V_H_
#define SRC_MODEL_V_H_

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
