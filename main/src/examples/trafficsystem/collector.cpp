/*
 * Collector.cpp
 *
 *  Created on: May 22, 2015
 *      Author: pieter
 */

#include "collector.h"

namespace n_examples_traffic {

CollectorState::CollectorState::CollectorState(std::string state)
{

}

CollectorState::CollectorState(const CollectorState&)
{

}

std::string CollectorState::toString()
{

}

Collector::Collector(): AtomicModel("Collector")
{

}

void Collector::extTransition(const std::vector<n_network::t_msgptr> & message)
{

}

std::shared_ptr<CollectorState> Collector::getCollectorState() const
{

}

}
