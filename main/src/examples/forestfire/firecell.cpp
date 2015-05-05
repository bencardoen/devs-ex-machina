/*
 * firecell.cpp
 *
 *  Created on: May 2, 2015
 *      Author: lttlemoi
 */

#include "firecell.h"
#include "objectfactory.h"
#include <cmath>

namespace n_examples {

std::string cellName(n_model::t_point pos)
{
	std::stringstream ssr;
	ssr << "Cell(" << pos.first << ", " << pos.second << ')';
	return ssr.str();
}

FireCell::FireCell(n_model::t_point pos):
	CellAtomicModel(cellName(pos), pos),
	m_myIports({{this->addInPort("in_N"),
		  this->addInPort("in_E"),
		  this->addInPort("in_S"),
		  this->addInPort("in_W"),
		  this->addInPort("in_G")}}),
	m_myOport(this->addOutPort("out_T"))
{
	setState(n_tools::createObject<FireCellState>());
	/*
	self.inports = [self.addInPort("in_N"), self.addInPort("in_E"), self.addInPort("in_S"), self.addInPort("in_W"), self.addInPort("in_G")]
        self.outport = self.addOutPort("out_T")
	 */
}

const FireCellState& FireCell::fcstate() const
{
	return *(std::dynamic_pointer_cast<FireCellState>(getState()));
}

FireCellState& FireCell::fcstate()
{
	return *(std::dynamic_pointer_cast<FireCellState>(getState()));
}

void FireCell::extTransition(const std::vector<n_network::t_msgptr>& message)
{
	/*
	if self.inports[-1] in inputs:
            # A temperature from the generator, so simply set our own temperature
            self.state.temperature = inputs[self.inports[-1]][0]
            self.state.phase = getPhaseFor(self.state.temperature, self.state.phase)
            if self.state.phase == PH_BURNING:
                self.state.igniteTime = self.state.currentTime * TIMESTEP
        else:
            for num, inport in enumerate(self.inports[:4]):
                self.state.surroundingTemps[num] = inputs.get(inport, [self.state.surroundingTemps[num]])[0]

        if self.state.phase == PH_INACTIVE:
            self.state.phase = PH_UNBURNED
	 */
	const FireCellState& state = fcstate();
	t_firecellstateptr newState = n_tools::createObject<FireCellState>(state);

	bool wasFromGenerator = false;
	for(const n_network::t_msgptr& msg: message){
		if(msg->getDestinationPort() == m_myIports[4]->getFullName()){
			wasFromGenerator = true;
			newState->m_temperature = *reinterpret_cast<const double*>(msg->getPayload().c_str());
			newState->m_phase = getNext(state.m_phase, newState->m_temperature);
			if(newState->m_phase == FirePhase::BURNING)
				newState->m_igniteTime = state.m_timeLast.getTime() * TIMESTEP;
		}
	}

	if(!wasFromGenerator){
		for(const n_network::t_msgptr& msg: message){
			double msgTemp = *reinterpret_cast<const double*>(msg->getPayload().c_str());
			switch(msg->getDestinationPort().back()){
			case 'N':
				newState->m_surroundingTemp[0] = msgTemp;
				break;
			case 'E':
				newState->m_surroundingTemp[1] = msgTemp;
				break;
			case 'S':
				newState->m_surroundingTemp[2] = msgTemp;
				break;
			case 'W':
				newState->m_surroundingTemp[3] = msgTemp;
				break;
			default:
				assert(false && "Unknown destination port for FireCell::extTransition");
				break;
			}
		}
	}

	if(newState->m_phase == FirePhase::INACTIVE)
		newState->m_phase = FirePhase::UNBURNED;
	setState(newState);
}

void FireCell::intTransition()
{
	/*
	if abs(self.state.temperature - self.state.oldTemperature) > TMP_DIFF:
            self.state.oldTemperature = self.state.temperature

        if self.state.phase == PH_BURNED:
            # Don't do anything as we are already finished
            return self.state
        elif self.state.phase == PH_BURNING:
            newTemp = 0.98689 * self.state.temperature + 0.0031 * (sum(self.state.surroundingTemps)) + 2.74 * exp(-0.19 * (self.state.currentTime * TIMESTEP - self.state.igniteTime)) + 0.213
        elif self.state.phase == PH_UNBURNED:
            newTemp = 0.98689 * self.state.temperature + 0.0031 * (sum(self.state.surroundingTemps)) + 0.213

        newPhase = getPhaseFor(newTemp, self.state.phase)
        if newPhase == PH_BURNED:
            newTemp = T_AMBIENT
        if self.state.phase == PH_UNBURNED and newPhase == PH_BURNING:
            self.state.igniteTime = self.state.currentTime * TIMESTEP

        self.state.phase = newPhase
        self.state.temperature = newTemp
	 */
	const FireCellState& state = fcstate();
	t_firecellstateptr newState = n_tools::createObject<FireCellState>(state);

	if(std::abs(state.m_temperature - state.m_oldTemp) > TMP_DIFF)
		newState->m_oldTemp = state.m_temperature;

	if(state.m_phase == FirePhase::BURNED){
		setState(getState());
		return;				//We're already finished burning
	}

	double newTemp = 0.98489 * state.m_temperature + 0.0031 * state.getSurroundingTemp() + 0.213;
	if (state.m_phase == FirePhase::BURNING){
		double ex = -0.19 * (state.m_timeLast.getTime() * TIMESTEP - (double) state.m_igniteTime);
		newTemp += 2.74 * std::exp(ex);
	}

	FirePhase newPhase = getNext(state.m_phase, newTemp);

	if(newPhase == FirePhase::BURNED)
		newTemp = T_AMBIENT;
	else if(state.m_phase == FirePhase::UNBURNED && newPhase == FirePhase::BURNING)
		newState->m_igniteTime = state.m_timeLast.getTime() * TIMESTEP;
	newState->m_phase = newPhase;
	newState->m_temperature = newTemp;

	std::cerr << newState->toString() << "\n";

	setState(newState);
}

t_timestamp FireCell::timeAdvance() const
{
	switch (fcstate().m_phase) {
	case FirePhase::INACTIVE:
		return t_timestamp::infinity();
	case FirePhase::UNBURNED:
		return t_timestamp(1);
	case FirePhase::BURNING:
		return t_timestamp(1);
	case FirePhase::BURNED:
	default:
		return t_timestamp::infinity();
	}
}

std::vector<n_network::t_msgptr> FireCell::output() const
{
	/*
	if abs(self.state.temperature - self.state.oldTemperature) > TMP_DIFF:
            return {self.outport: [self.state.temperature]}
        else:
            return {}
	 */
	const FireCellState& state = fcstate();
	if(std::abs(state.m_temperature - state.m_oldTemp) > TMP_DIFF) {
		std::string msg(reinterpret_cast<const char*>(&(state.m_temperature)), sizeof(double));
//		const char* msgContent = reinterpret_cast<const char*>(&(state.m_temperature));
//		for(std::size_t i = 0; i < sizeof(double); ++i)
//			msg[i] = msgContent[i];
		return m_myOport->createMessages(msg);
	}
	return std::vector<n_network::t_msgptr>();
}

} /* namespace n_examples */
