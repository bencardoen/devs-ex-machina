/*
 * LocationTable.h
 *
 *  Created on: 11 Mar 2015
 *      Author: matthijs
 */

#ifndef SRC_CONTROL_LOCATIONTABLE_H_
#define SRC_CONTROL_LOCATIONTABLE_H_

#include <unordered_map>
//#include "model.h"
#include "core.h"

using n_model::t_modelptr;
// points to stubbed class in core.h

namespace n_control {

class LocationTable
{
public:
	LocationTable(std::size_t amountCores);
	virtual ~LocationTable();

	std::size_t lookupModel(std::string modelName);
	void registerModel(const t_modelptr& model, std::size_t core);

private:
	std::size_t m_amountCores;
	std::unordered_map<std::string, std::size_t> m_locTable;
};

} /* namespace n_control */

#endif /* SRC_CONTROL_LOCATIONTABLE_H_ */
