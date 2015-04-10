/*
 * v.h
 *
 *  Created on: 10 Apr 2015
 *      Author: Ben Cardoen
 */

#ifndef SRC_MODEL_V_H_
#define SRC_MODEL_V_H_

namespace n_model {

/**
 * Thread-shared messagecount vector.
 */
class V
{
private:
	std::vector<int>	m_vec;
public:
	std::vector<int>&
	getVector(){return m_vec;}
	V();
	virtual ~V();
};

typedef std::shared_ptr<V> t_V;

} /* namespace n_model */

#endif /* SRC_MODEL_V_H_ */
