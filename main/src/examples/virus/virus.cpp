/*
 * virus.cpp
 *
 *  Created on: 27 May 2015
 *      Author: matthijs
 */

#include "examples/virus/virus.h"

namespace n_virus {

std::ostream& operator <<(std::ostream& out, const MsgData& data)
{
	return (out << data.m_value);
}

MsgData::MsgData(int val, std::string from):
	m_value(val), m_from(from)
{
}


/*
 * CELLSTATE
 */

CellState::CellState(size_t production, int capacity)
	: m_production(production), m_capacity(capacity), m_toSend(0), m_target(0u),
	 m_produced(false)
{
}


/*
 * CELL
 */

Cell::Cell(std::string name, size_t neighbours, size_t production, size_t seed, size_t capacity)
	: AtomicModel<CellState>(name, CellState(production, capacity))
{
	m_rng.seed(seed);
	addInPort("in");
}

n_model::t_portptr n_virus::Cell::addConnection()
{
	n_model::t_portptr ptr = addOutPort("out_" + n_tools::toString(m_neighbours.size()));
	m_neighbours.push_back(ptr);
	return ptr;
}

Cell::~Cell()
{
}

int Cell::send()
{
	if (abs(state().m_capacity) < 2)
		return 0;

	std::uniform_int_distribution<int> dist(std::min(0,state().m_capacity-1), std::max(0,state().m_capacity-1));
	return dist(m_rng);
}

void Cell::receive(int incoming)
{
	state().m_capacity += incoming;
}

void Cell::produce()
{
	state().m_capacity += n_tools::sgn(state().m_capacity) * state().m_production;
}

n_model::t_timestamp Cell::timeAdvance() const
{
	return state().m_capacity == 0? n_network::t_timestamp::infinity() : n_network::t_timestamp(10, 0);
}

void Cell::intTransition()
{
	produce();
	state().m_capacity -= state().m_toSend;
	state().m_produced = true;

	std::uniform_int_distribution<size_t> dist(0, m_neighbours.size()-1);
	state().m_target = dist(m_rng);
	state().m_toSend = send();	//calculate what we'll send next time round
}

void Cell::confTransition(const std::vector<n_network::t_msgptr> & message)
{
	produce();
	state().m_capacity -= state().m_toSend;
	state().m_produced = true;
	for (auto& msg : message) {
		int incoming = n_network::getMsgPayload<MsgData>(msg).m_value;
		receive(incoming);
	}

	std::uniform_int_distribution<size_t> dist(0, m_neighbours.size()-1);
	state().m_target = dist(m_rng);
	state().m_toSend = send();	//calculate what we'll send next time round
}

void Cell::extTransition(const std::vector<n_network::t_msgptr>& message)
{
	LOG_DEBUG("virus external transition");
	state().m_produced = false;
	for (auto& msg : message) {
		int incoming = n_network::getMsgPayload<MsgData>(msg).m_value;
		receive(incoming);
	}
	state().m_toSend = send();	//calculate what we'll send next time round
}

void Cell::output(std::vector<n_network::t_msgptr>& msgs) const
{
	if (state().m_toSend != 0) {
		m_neighbours[state().m_target]->createMessages(MsgData(state().m_toSend, getName()), msgs);
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

	std::vector<std::shared_ptr<Cell>> cells = { posBase, negBase };
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

			connectPorts(cells[thisCell]->addConnection(),
			        cells[otherCell]->getPort("in"));
			connectPorts(cells[otherCell]->addConnection(),
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
