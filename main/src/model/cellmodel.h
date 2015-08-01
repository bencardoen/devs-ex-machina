/*
 * cellmodel.h
 *
 *  Created on: May 1, 2015
 *      Author: Stijn Manhaeve - Devs Ex Machina
 */

#ifndef SRC_MODEL_CELLMODEL_H_
#define SRC_MODEL_CELLMODEL_H_

#include <utility>
#include "model/atomicmodel.h"

namespace n_model {

typedef std::pair<std::size_t, std::size_t> t_point;

/**
 * @brief Specialized atomic model that contains a position.
 * This model can be used for the Cell Tracer
 */
class CellAtomicModel: public n_model::AtomicModel
{
protected:
	t_point m_pos;

public:
	CellAtomicModel() = delete;
	CellAtomicModel(const CellAtomicModel&) = default;
	CellAtomicModel(std::string name, std::size_t priority = 0);
	CellAtomicModel(std::string name, t_point point, std::size_t priority = 0);

	virtual ~CellAtomicModel() = default;

	/**
	 * @return The position property of this state
	 * @see t_point
	 */
	const t_point& getPoint() const;
	/**
	 * @return The position property of this state
	 * @see t_point
	 */
	t_point getPoint();

	/**
	 * @brief Sets the position property of this state
	 */
	void setPoint(t_point pt);
};

typedef std::shared_ptr<CellAtomicModel> t_cellmodelptr;

} /* namespace n_model */

#endif /* SRC_MODEL_CELLMODEL_H_ */
