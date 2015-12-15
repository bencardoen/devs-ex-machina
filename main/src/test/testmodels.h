/*
 * This file is part of the DEVS Ex Machina project.
 * Copyright 2014 - 2015 University of Antwerp
 * https://www.uantwerpen.be/en/
 * Licensed under the EUPL V.1.1
 * A full copy of the license is in COPYING.txt, or can be found at
 * https://joinup.ec.europa.eu/community/eupl/og_page/eupl
 *      Author: Stijn Manhaeve, Tim Tuijn
 */

#ifndef SRC_TEST_TESTMODELS_H_
#define SRC_TEST_TESTMODELS_H_

#include "model/atomicmodel.h"
#include "model/coupledmodel.h"
#include "performance/devstone/devstone.h"

namespace n_testmodel
{
struct Event
{
	std::size_t m_eventSize;
};

std::ostream& operator<<(std::ostream& stream, const Event& event);

struct ModelState
{
	std::size_t m_counter;	//keep this public as these are just for test models
	Event m_event;

	ModelState();
};

class Processor: public n_model::AtomicModel<ModelState>
{
public:
	std::size_t m_event1;
	n_model::t_portptr m_inport;
	n_model::t_portptr m_outport;
	Processor(std::string name = "Processor", std::size_t t_event1 = 10u);

	virtual void extTransition(const std::vector<n_network::t_msgptr> & message) override;
	virtual void intTransition() override;
	virtual n_network::t_timestamp timeAdvance() const override;
	virtual void output(std::vector<n_network::t_msgptr>& msgs) const override;
};

class GeneratorState: public ModelState
{
public:
	std::size_t m_generated;
	std::size_t m_value;

	GeneratorState();
	GeneratorState(const GeneratorState& other) = default;
};

class Generator: public n_model::AtomicModel<GeneratorState>
{
public:
	std::size_t m_gen_event1;
	bool m_binary;
	n_model::t_portptr m_inport;
	n_model::t_portptr m_outport;
	Generator(std::string name = "Processor", std::size_t t_gen_event1 = 10u, bool binary = false);

	virtual void extTransition(const std::vector<n_network::t_msgptr> & message) override;
	virtual void intTransition() override;
	virtual n_network::t_timestamp timeAdvance() const override;
	virtual void output(std::vector<n_network::t_msgptr>& msgs) const override;
};

class CoupledProcessor: public n_model::CoupledModel
{
public:
	n_model::t_portptr m_inport;
	n_model::t_portptr m_outport;
	CoupledProcessor(std::size_t event1_P1, std::size_t levels);
};

class ElapsedNothing: public n_model::AtomicModel<ModelState>
{
public:
	ElapsedNothing();
	virtual void intTransition() override;
	virtual n_network::t_timestamp timeAdvance() const override;
	virtual void output(std::vector<n_network::t_msgptr>&) const override
	{
		//nothing to do here
	}
};

class GeneratorDS: public Generator
{
public:
	GeneratorDS();
	virtual void output(std::vector<n_network::t_msgptr>& msgs) const override;

	virtual bool modelTransition(n_model::DSSharedState* shared) override;

};

class DSDevsRoot: public n_model::CoupledModel
{
public:
	std::shared_ptr<GeneratorDS> m_model;
	std::shared_ptr<Processor> m_model2;
	std::shared_ptr<Processor> m_model3;
	std::shared_ptr<CoupledProcessor> m_model4;
	std::shared_ptr<ElapsedNothing> m_modelX;
	DSDevsRoot();
	virtual bool modelTransition(n_model::DSSharedState* shared) override;
};

} /* namespace n_testmodel */
template<>
struct ToString<n_testmodel::ModelState>
{
	static std::string exec(const n_testmodel::ModelState& s){
		return s.m_counter == std::numeric_limits<std::size_t>::max() ? "inf": n_tools::toString(s.m_counter);
	}
};
template<>
struct ToXML<n_testmodel::ModelState>
{
	static std::string exec(const n_testmodel::ModelState& s){
		return std::string("<counter>") + ToString<n_testmodel::ModelState>::exec(s) + "</counter>";
	}
};
template<>
struct ToString<n_testmodel::GeneratorState>
{
	static std::string exec(const n_testmodel::GeneratorState& s){
		return s.m_counter == std::numeric_limits<std::size_t>::max() ? "inf": n_tools::toString(s.m_counter);
	}
};
template<>
struct ToXML<n_testmodel::GeneratorState>
{
	static std::string exec(const n_testmodel::GeneratorState& s){
		return std::string("<counter>") + ToString<n_testmodel::ModelState>::exec(s) + "</counter>";
	}
};

#endif /* SRC_TEST_TESTMODELS_H_ */
