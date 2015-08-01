/*
 * firespread.h
 *
 *  Created on: May 4, 2015
 *      Author: lttlemoi
 */

#ifndef SRC_EXAMPLES_FORESTFIRE_FIRESPREAD_H_
#define SRC_EXAMPLES_FORESTFIRE_FIRESPREAD_H_

#include "model/coupledmodel.h"

namespace n_examples {

class FireSpread: public n_model::CoupledModel
{
public:
	FireSpread(std::size_t sizeX, std::size_t sizeY);
};

} /* namespace n_examples */

#endif /* SRC_EXAMPLES_FORESTFIRE_FIRESPREAD_H_ */
