/*
 * firecell.cpp
 *
 *  Created on: May 2, 2015
 *      Author: lttlemoi
 */

#include "examples/forestfire/firecell.h"
#include "tools/objectfactory.h"
#include <cmath>

namespace n_examples {

std::string cellName(n_model::t_point pos)
{
	std::stringstream ssr;
	ssr << "Cell(" << pos.first << ", " << pos.second << ')';
	return ssr.str();
}

FireCell::FireCell(n_model::t_point pos):
	CellAtomicModel<FireCellState>(cellName(pos), pos, FireCellState()),
	m_myIports({{this->addInPort("in_N"),
		  this->addInPort("in_E"),
		  this->addInPort("in_S"),
		  this->addInPort("in_W"),
		  this->addInPort("in_G")}}),
	m_myOport(this->addOutPort("out_T"))
{
	/*
	self.inports = [self.addInPort("in_N"), self.addInPort("in_E"), self.addInPort("in_S"), self.addInPort("in_W"), self.addInPort("in_G")]
        self.outport = self.addOutPort("out_T")
	 */
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
	FireCellState& st = state();

	bool wasFromGenerator = false;
	for(const n_network::t_msgptr& msg: message){
		if(msg->getDestinationPort() == 4){
			wasFromGenerator = true;
			st.m_temperature = n_network::getMsgPayload<double>(msg);
			st.m_phase = getNext(st.m_phase, st.m_temperature);
			if(st.m_phase == FirePhase::BURNING)
				st.m_igniteTime = getTimeLast().getTime() * TIMESTEP;
		}
	}

	if(!wasFromGenerator){
		for(const n_network::t_msgptr& msg: message){
			double msgTemp = n_network::getMsgPayload<double>(msg);
			switch(msg->getDestinationPort()){
			case 0u:
				st.m_surroundingTemp[0] = msgTemp;
				break;
			case 1u:
				st.m_surroundingTemp[1] = msgTemp;
				break;
			case 2u:
				st.m_surroundingTemp[2] = msgTemp;
				break;
			case 3u:
				st.m_surroundingTemp[3] = msgTemp;
				break;
			default:
				assert(false && "Unknown destination port for FireCell::extTransition");
				break;
			}
		}
	}

	if(st.m_phase == FirePhase::INACTIVE)
		st.m_phase = FirePhase::UNBURNED;
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
	FireCellState& st = state();

	if(std::abs(st.m_temperature - st.m_oldTemp) > TMP_DIFF)
		st.m_oldTemp = st.m_temperature;

	if(st.m_phase == FirePhase::BURNED){
		return;				//We're already finished burning
	}

	double newTemp = 0.98489 * st.m_temperature + 0.0031 * st.getSurroundingTemp() + 0.213;
	if (st.m_phase == FirePhase::BURNING){
		double ex = -0.19 * (getTimeLast().getTime() * TIMESTEP - (double) st.m_igniteTime);
		newTemp += 2.74 * std::exp(ex);
	}

	FirePhase newPhase = getNext(st.m_phase, newTemp);

	if(newPhase == FirePhase::BURNED)
		newTemp = T_AMBIENT;
	else if(st.m_phase == FirePhase::UNBURNED && newPhase == FirePhase::BURNING)
		st.m_igniteTime = getTimeLast().getTime() * TIMESTEP;
	st.m_phase = newPhase;
	st.m_temperature = newTemp;
}

t_timestamp FireCell::timeAdvance() const
{
	switch (state().m_phase) {
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

void FireCell::output(std::vector<n_network::t_msgptr>& msgs) const
{
	/*
	if abs(self.state.temperature - self.state.oldTemperature) > TMP_DIFF:
            return {self.outport: [self.state.temperature]}
        else:
            return {}
	 */
	if(std::abs(state().m_temperature - state().m_oldTemp) > TMP_DIFF) {
		m_myOport->createMessages(state().m_temperature, msgs);
	}
}

} /* namespace n_examples */
