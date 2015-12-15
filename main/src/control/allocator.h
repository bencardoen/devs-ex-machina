/*
 * This file is part of the DEVS Ex Machina project.
 * Copyright 2014 - 2015 University of Antwerp 
 * https://www.uantwerpen.be/en/
 * Licensed under the EUPL V.1.1
 * A full copy of the license is in COPYING.txt, or can be found at 
 * https://joinup.ec.europa.eu/community/eupl/og_page/eupl 
 *      Author: Matthijs Van Os, Stijn Manhaeve. 
 */

#ifndef SRC_CONTROL_ALLOCATOR_H_
#define SRC_CONTROL_ALLOCATOR_H_

#include <memory>
#include <vector>
#include "model/atomicmodel.h"
#include "control/simtype.h"

namespace n_control {

/**
 * @brief Decides on which Core to place each Model
 */
class Allocator
{
private:
	std::size_t m_numcores;
	SimType m_simtype;

protected:
	/**
	 * @brief Assigns a core id to a model and checks whether it is allowed.
	 */
	inline void assignCore(const n_model::t_atomicmodelptr& model, std::size_t id) const
	{
		assert(id < m_numcores && "Assigned invalid core number.");
		model->setCorenumber(id);
	}
public:
	Allocator();

	/**
	 * @brief Returns on which Core to place a Model
	 * @param model the AtomicModel to be allocated
	 * @deprecated
	 */
	virtual size_t allocate(const n_model::t_atomicmodelptr&)__attribute__((deprecated)) = 0;

	/**
	 * @brief Allocates all models.
	 * @param models A vector of AtomicModels that need to be allocated.
	 * @postcondition All models have been assigned a core id. @see assignCore.
	 */
	virtual void allocateAll(const std::vector<n_model::t_atomicmodelptr>&)=0;

	/**
	 * @brief setter for the amount of simulation cores. The allocater must distribute the models among these cores.
	 * @param amount The new amount.
	 * @precondition amount is not zero.
	 */
	void setCoreAmount(std::size_t amount);

	/**
	 * @brief setter for the simulation type.
	 * @param type A SimType enum value.
	 */
	void setSimType(SimType type);

	/**
	 * @return the configured amount of simulation cores.
	 * @see setCoreAmount
	 */
	inline std::size_t coreAmount() const
	{
		return m_numcores;
	}

	/**
	 * @return the configured simulation type
	 * @see setSimType
	 */
	inline SimType simType() const
	{
		return m_simtype;
	}

	virtual ~Allocator();
};

} /* namespace n_control */

#endif /* SRC_CONTROL_ALLOCATOR_H_ */
