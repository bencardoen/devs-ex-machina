/*
 * virus.h
 *
 *  Created on: 27 May 2015
 *      Author: matthijs
 */

#ifndef SRC_EXAMPLES_VIRUS_VIRUS_H_
#define SRC_EXAMPLES_VIRUS_VIRUS_H_

#include <random>
#include <chrono>
#include <algorithm>
#include "misc.h"
#include "state.h"
#include "objectfactory.h"
#include "atomicmodel.h"
#include "coupledmodel.h"

namespace n_virus {

class CellState: public n_model::State
{
public:
	size_t m_production;
	int m_capacity;
public:
	CellState(size_t production, int capacity = 0);
	virtual ~CellState();
	virtual std::string toString();
};

class Cell: public n_model::AtomicModel
{
private:
	std::mt19937 m_rng;
	std::vector<n_model::t_portptr> m_neighbours;
	std::shared_ptr<CellState> m_curState;
	int m_leaving;
	size_t m_target;
public:
	Cell(std::string name, size_t neighbours, size_t production, size_t seed, size_t capacity = 0);
	virtual ~Cell();

	void updateCurState();
	int send();
	void receive(int incoming);
	void produce();

	virtual n_model::t_timestamp timeAdvance() const;
	virtual void intTransition();
	virtual void confTransition(const std::vector<n_network::t_msgptr> & message);
	virtual void extTransition(const std::vector<n_network::t_msgptr>& message);
	virtual std::vector<n_network::t_msgptr> output() const;
};

class Structure: public n_model::CoupledModel
{
private:
	std::mt19937 m_rng;
public:
	Structure(size_t poolsize, size_t connections);
	virtual ~Structure();
};

} /* namespace n_virus */

#endif /* SRC_EXAMPLES_VIRUS_VIRUS_H_ */
