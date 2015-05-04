/*
 * firegenerator.h
 *
 *  Created on: May 4, 2015
 *      Author: lttlemoi
 */

#ifndef SRC_EXAMPLES_FORESTFIRE_FIREGENERATOR_H_
#define SRC_EXAMPLES_FORESTFIRE_FIREGENERATOR_H_

#include <atomicmodel.h>

namespace n_examples {

class FireGenerator;

class FireGeneratorState: public n_model::State
{
	friend FireGenerator;
private:
	bool m_status;
public:
	FireGeneratorState();

	std::string toString() override;
};

typedef std::shared_ptr<FireGeneratorState> t_fgstateptr;

class FireGenerator: public n_model::AtomicModel
{
private:
	FireGeneratorState& getFGState();
	const FireGeneratorState& getFGState() const;

	std::vector<n_model::t_portptr> m_outputs;
public:
	FireGenerator(std::size_t levels);

	void intTransition() override;
	n_network::t_timestamp timeAdvance() const override;
	std::vector<n_network::t_msgptr> output() const override;

	std::vector<n_model::t_portptr>& getOutputs();
};

typedef std::shared_ptr<FireGenerator> t_fgenptr;

} /* namespace n_examples */

#endif /* SRC_EXAMPLES_FORESTFIRE_FIREGENERATOR_H_ */
