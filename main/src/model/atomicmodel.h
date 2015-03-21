/*
 * atomicModel.h
 *
 *  Created on: 9-mrt.-2015
 *      Author: Pieter, Tim
 */

#ifndef ATOMICMODEL_H_
#define ATOMICMODEL_H_

#include "model.h"
#include "message.h"
#include <map>
#include <iostream>

namespace n_model {
class AtomicModel : public Model
{
protected:
	// lower number -> higher priority
	std::size_t m_priority;
	t_timestamp m_elapsed;
	t_timestamp m_lastRead;

public:
	AtomicModel() = delete;
	AtomicModel(std::string name, int corenumber, std::size_t priority = 0);

	virtual void extTransition(const std::vector<n_network::t_msgptr> & message) = 0;
	virtual void intTransition() = 0;
	virtual void confTransition(const std::vector<n_network::t_msgptr> & message);
	virtual t_timestamp timeAdvance() = 0;
	virtual std::vector<n_network::t_msgptr> output() = 0;
	void setGVT(t_timestamp gvt);
	void revert(t_timestamp time);
	std::size_t getPriority() const;

	virtual ~AtomicModel() {}
};

typedef std::shared_ptr<AtomicModel> t_atomicmodelptr;
}

#endif /* ATOMICMODEL_H_ */
