/*
 * virus.cpp
 *
 *  Created on: 27 May 2015
 *      Author: matthijs
 */

#include "virus.h"

namespace n_virus {

/*
 * CELLSTATE
 */

CellState::CellState(size_t production, int capacity)
	: m_production(production), m_capacity(capacity)
{
}

CellState::~CellState()
{
}

std::string CellState::toString()
{
	return "[Production: "+n_tools::toString(m_production)+" Capacity: "+n_tools::toString(m_capacity)+"]";
}


/*
 * CELL
 */

Cell::Cell(std::string name, size_t neighbours, size_t production, size_t seed, size_t capacity)
	: AtomicModel(name), m_leaving(0), m_target(0)
{
	m_rng.seed(seed);
	addInPort("in");
	for (size_t i = 0; i < neighbours; ++i) {
		m_neighbours.push_back(addOutPort("out_" + n_tools::toString(i)));
	}
	setState(n_tools::createObject<CellState>(production, capacity));
}

Cell::~Cell()
{
}

void Cell::updateCurState()
{
	m_curState = n_tools::createObject<CellState>(*std::dynamic_pointer_cast<CellState>(getState()));
}

int Cell::send()
{
	if (abs(m_curState->m_capacity) < 2)
		return 0;

	std::uniform_int_distribution<int> dist(std::min(0,m_curState->m_capacity-1), std::max(0,m_curState->m_capacity-1));
	int amount = dist(m_rng);
	m_curState->m_capacity -= amount;
	return amount;
}

void Cell::receive(int incoming)
{
	m_curState->m_capacity += incoming;
}

void Cell::produce()
{
	m_curState->m_capacity += n_tools::sgn(m_curState->m_capacity) * m_curState->m_production;
}

n_model::t_timestamp Cell::timeAdvance() const
{
	return n_network::t_timestamp(10, 0);
}

void Cell::intTransition()
{
	updateCurState();
	produce();

	// We need to precalculate the following since output is const
	m_leaving = send();
	std::uniform_int_distribution<size_t> dist(0, m_neighbours.size()-1);
	m_target = dist(m_rng);

	setState(m_curState);
}

void Cell::confTransition(const std::vector<n_network::t_msgptr> & message)
{
	updateCurState();
	produce();
	for (auto& msg : message) {
		int incoming = n_network::getMsgPayload<int>(msg);
		receive(incoming);
	}
	m_leaving = send();
	setState(m_curState);
}

void Cell::extTransition(const std::vector<n_network::t_msgptr>& message)
{
	updateCurState();
	for (auto& msg : message) {
		int incoming = n_network::getMsgPayload<int>(msg);
		receive(incoming);
	}
	setState(m_curState);
}

std::vector<n_network::t_msgptr> Cell::output() const
{
	if (m_leaving != 0) {
		auto messages = m_neighbours[m_target]->createMessages(m_leaving);
		return messages;
	} else {
		return std::vector<n_network::t_msgptr>();
	}
}


/*
 * STRUCTURE
 */

Structure::Structure(size_t poolsize, size_t connections)
	: CoupledModel("Virus")
{
	m_rng.seed(std::chrono::system_clock::now().time_since_epoch().count());
	std::uniform_int_distribution<size_t> seeder;
	auto posBase = n_tools::createObject<Cell>("PosBase", connections, 12, seeder(m_rng), 60);
	auto negBase = n_tools::createObject<Cell>("NegBase", connections, 12, seeder(m_rng), -60);
	addSubModel(posBase);
	addSubModel(negBase);

	std::vector<n_model::t_atomicmodelptr> cells = { posBase, negBase };
	std::uniform_int_distribution<size_t> sizer(1, 12);
	for (size_t i = 0; i < poolsize; ++i) {
		cells.push_back(
		        n_tools::createObject<Cell>("Cell_" + n_tools::toString(i), connections, sizer(m_rng),
		                seeder(m_rng)));
		addSubModel(cells[2+i]);
	}

	std::vector<std::vector<size_t>> conGrid; // Connection grid for random connections
	for (size_t i = 0; i < poolsize + 2; ++i) {
		conGrid.push_back(std::vector<size_t>());
		for (size_t j = 0; j < connections; ++j) {
			conGrid[i].push_back(j);
		}
	}

	for (size_t thisCell = 0; thisCell < cells.size(); ++thisCell) {
		if(thisCell == conGrid.size()-1) break; // No connections left anyway
		std::uniform_int_distribution<size_t> connector(thisCell+1, conGrid.size()-1);
		for (size_t i = 0; i < conGrid[thisCell].size(); ++i) {
			size_t otherCell = connector(m_rng);
			while (conGrid[otherCell].empty()) {
				++otherCell;
				otherCell %= conGrid.size();
			}
			size_t thisPort = conGrid[thisCell].back();
			size_t otherPort = conGrid[otherCell].back();

			connectPorts(cells[thisCell]->getPort("out_" + n_tools::toString(thisPort)),
			        cells[otherCell]->getPort("in"));
			connectPorts(cells[otherCell]->getPort("out_" + n_tools::toString(otherPort)),
			        cells[thisCell]->getPort("in"));

			conGrid[otherCell].pop_back();
			conGrid[thisCell].pop_back();
		}
	}
}

Structure::~Structure()
{
}

} /* namespace n_virus */

