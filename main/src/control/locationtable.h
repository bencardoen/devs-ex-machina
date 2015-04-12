/*
 * LocationTable.h
 *
 *  Created on: 11 Mar 2015
 *      Author: matthijs
 */

#ifndef SRC_CONTROL_LOCATIONTABLE_H_
#define SRC_CONTROL_LOCATIONTABLE_H_

#include <unordered_map>
#include "atomicmodel.h"

namespace n_control {

using n_model::t_atomicmodelptr;

class LocationTable
{
public:
	LocationTable(std::size_t amountCores);
	virtual ~LocationTable();

	std::size_t lookupModel(const std::string& modelName);
	void registerModel(const t_atomicmodelptr& model, std::size_t core);

private:
	const std::size_t m_amountCores;
	std::unordered_map<std::string, std::size_t> m_locTable;
};

typedef std::shared_ptr<LocationTable> t_location_tableptr;

} /* namespace n_control */

#endif /* SRC_CONTROL_LOCATIONTABLE_H_ */
