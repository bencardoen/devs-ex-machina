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
#include "tools/misc.h"
#include "model/state.h"
#include "tools/objectfactory.h"
#include "model/atomicmodel.h"
#include "model/coupledmodel.h"

namespace n_virus {

/**
 * @brief Small struct that is sent as a message.
 *
 * It contains an amoint of viri and the name of the source cell
 */
struct MsgData
{
	int m_value;
	std::string m_from;

	MsgData(int val, std::string from);
};

std::ostream& operator<<(std::ostream& out, const MsgData& data);

class CellState
{
public:
	size_t m_production;
	int m_capacity;
	int m_toSend;	//how much will be sent the this time
	std::size_t m_target; //the target
	bool m_produced;		//whether or not we produced something last time

	CellState(size_t production, int capacity = 0);
};

} /* namespace n_virus */

template<>
struct ToString<n_virus::CellState>
{
	static std::string exec(const n_virus::CellState& s){
		return "[Production: "+n_tools::toString(s.m_production)+" Capacity: "+n_tools::toString(s.m_capacity)+"]";
	}
};

namespace n_virus {

class Cell: public n_model::AtomicModel<CellState>
{
private:
	std::mt19937 m_rng;
	std::vector<n_model::t_portptr> m_neighbours;
public:
	Cell(std::string name, size_t neighbours, size_t production, size_t seed, size_t capacity = 0);
	virtual ~Cell();

	int send();
	void receive(int incoming);
	void produce();

	n_model::t_portptr addConnection();

	virtual n_model::t_timestamp timeAdvance() const;
	virtual void intTransition();
	virtual void confTransition(const std::vector<n_network::t_msgptr> & message);
	virtual void extTransition(const std::vector<n_network::t_msgptr>& message);
	virtual void output(std::vector<n_network::t_msgptr>& msgs) const;
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
