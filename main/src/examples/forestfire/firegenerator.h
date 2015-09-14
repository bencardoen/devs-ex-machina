/*
 * firegenerator.h
 *
 *  Created on: May 4, 2015
 *      Author: lttlemoi
 */

#ifndef SRC_EXAMPLES_FORESTFIRE_FIREGENERATOR_H_
#define SRC_EXAMPLES_FORESTFIRE_FIREGENERATOR_H_

#include "model/atomicmodel.h"

namespace n_examples {

struct FGState
{
	bool m_value;
	FGState(bool v = false): m_value(v)
	{}
};

class FireGenerator: public n_model::AtomicModel<FGState>
{
private:
	std::vector<n_model::t_portptr> m_outputs;
public:
	FireGenerator(std::size_t levels);

	void intTransition() override;
	n_network::t_timestamp timeAdvance() const override;
	void output(std::vector<n_network::t_msgptr>& msgs) const override;

	std::vector<n_model::t_portptr>& getOutputs();
};

typedef std::shared_ptr<FireGenerator> t_fgenptr;

} /* namespace n_examples */


template<>
struct ToString<n_examples::FGState>
{
	static std::string exec(const n_examples::FGState& s){
		 return (s.m_value)? "GeneratorState: true": "GeneratorState: false";
	}
};
template<>
struct ToCell<n_examples::FGState>
{
	static std::string exec(const n_examples::FGState&){
		 return "";
	}
};

#endif /* SRC_EXAMPLES_FORESTFIRE_FIREGENERATOR_H_ */
