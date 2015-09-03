/*
 * firecell.h
 *
 *  Created on: May 2, 2015
 *      Author: lttlemoi
 */

#ifndef SRC_EXAMPLES_FORESTFIRE_FIRECELL_H_
#define SRC_EXAMPLES_FORESTFIRE_FIRECELL_H_

#include "model/cellmodel.h"
#include "examples/forestfire/firecellstate.h"

using namespace n_network;

namespace n_examples {

class FireCell: public n_model::CellAtomicModel
{
private:
	const FireCellState& fcstate() const;
	FireCellState& fcstate();

	std::array<n_model::t_portptr, 5> m_myIports;
	n_model::t_portptr m_myOport;
public:
	FireCell(n_model::t_point pos);

	void extTransition(const std::vector<n_network::t_msgptr> & message) override;
	void intTransition() override;
	t_timestamp timeAdvance() const override;
	void output(std::vector<n_network::t_msgptr>& msgs) const override;
};

typedef std::shared_ptr<FireCell> t_firecellptr;

} /* namespace n_examples */

#endif /* SRC_EXAMPLES_FORESTFIRE_FIRECELL_H_ */
