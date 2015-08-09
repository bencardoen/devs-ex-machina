/*
 * LocationTable.h
 *
 *  Created on: 11 Mar 2015
 *      Author: matthijs
 */

#ifndef SRC_CONTROL_LOCATIONTABLE_H_
#define SRC_CONTROL_LOCATIONTABLE_H_

#include <unordered_map>
#include "model/atomicmodel.h"

namespace n_control {

using n_model::t_atomicmodelptr;

/**
 * @brief Keeps the location of all Models
 */
class LocationTable
{
public:
	LocationTable(std::size_t amountCores);
	virtual ~LocationTable();

	/**
	 * @brief Return the location of a Model
	 * @throws out_of_range if modelName is not registered.
	 */
	std::size_t lookupModel(const std::string& modelName);

	/**
	 * @brief Register the location of a Model
	 * @param model the Model to be registered
	 * @param core the ID of the core on which the model is located
	 * @pre core < m_amountCores (assert)
	 */
	void registerModel(const t_atomicmodelptr& model, std::size_t core);


private:
	/**
	 * How many cores exist in total
	 */
	const std::size_t m_amountCores;

	/**
	 * Stores the location of all Models
	 */
	std::unordered_map<std::string, std::size_t> m_locTable;
};

typedef std::shared_ptr<LocationTable> t_location_tableptr;

} /* namespace n_control */

#endif /* SRC_CONTROL_LOCATIONTABLE_H_ */
