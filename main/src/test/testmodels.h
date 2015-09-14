/*
 * testmodels.h
 *
 *  Created on: May 9, 2015
 *      Author: Stijn Manhaeve - Devs Ex Machina
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

class ModelState: public n_model::State
{
public:
	std::size_t m_counter;	//keep this public as these are just for test models
	Event m_event;

	ModelState();
	ModelState(const ModelState& other) = default;

	std::string toString() override;
	std::string toXML() override;
};

class Processor: public n_model::AtomicModel_impl
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

class Generator: public n_model::AtomicModel_impl
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

class ElapsedNothing: public n_model::AtomicModel_impl
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

#endif /* SRC_TEST_TESTMODELS_H_ */
