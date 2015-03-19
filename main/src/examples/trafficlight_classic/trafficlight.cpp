/*
 * trafficlight.cpp
 *
 *  Created on: Mar 19, 2015
 *      Author: tim
 */

#include "trafficlight.h"

namespace n_examples {

TrafficLightMode::TrafficLightMode(e_colors color)
	: m_color(color)
{

}
std::string TrafficLightMode::toString()
{
	switch (m_color) {
	case RED:
		return "Red";
	case GREEN:
		return "Green";
	case YELLOW:
		return "Yellow";
	}
	assert(false); // This should never happen!
	return "";
}

std::string TrafficLightMode::toXML()
{
	return "<state color =\"" + this->toString() + "\"/>";
}

std::string TrafficLightMode::toJSON()
{
	return "{ \"state\": \"" + this->toString() + "\" }";
}

std::string TrafficLightMode::toCell()
{
	return "";
}

TrafficLight::TrafficLight(std::string name, std::size_t priority)
	: AtomicModel(name, priority)
{
	m_state = std::make_shared<TrafficLightMode>(RED);
	// Initialize elapsed attribute if required
	m_elapsed = 0;
}

void TrafficLight::extTransition(const t_msgptr & message)
{
	// No external transitions available yet...
}

void TrafficLight::intTransition()
{
	std::shared_ptr<TrafficLightMode> state;

	state = std::dynamic_pointer_cast<TrafficLightMode>(m_state);

	switch (state->m_color) {
	case RED:
		state->m_color = GREEN;
		return;
	case GREEN:
		state->m_color = YELLOW;
		return;
	case YELLOW:
		state->m_color = RED;
		return;
	}
}

t_timestamp TrafficLight::timeAdvance()
{
	std::shared_ptr<TrafficLightMode> state;

	state = std::dynamic_pointer_cast<TrafficLightMode>(m_state);

	switch(state->m_color) {
	case RED:
		return t_timestamp(60);
	case GREEN:
		return t_timestamp(50);
	case YELLOW:
		return t_timestamp(10);
	}
	assert(false); // You shouldn't come here...
	return t_timestamp();
}

std::map<t_portptr, t_msgptr> TrafficLight::output()
{
	// TODO not implement?
	return std::map<t_portptr, t_msgptr>();
}

}

