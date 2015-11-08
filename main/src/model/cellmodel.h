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
class CellAtomicModel_impl: public n_model::AtomicModel_impl
{
protected:
	t_point m_pos;

public:
	CellAtomicModel_impl() = delete;
	CellAtomicModel_impl(std::string name, int core, std::size_t priority = 0);
	CellAtomicModel_impl(std::string name, t_point point, int core, std::size_t priority = 0);
	CellAtomicModel_impl(std::string name, std::size_t priority = 0);
	CellAtomicModel_impl(std::string name, t_point point, std::size_t priority = 0);

	virtual ~CellAtomicModel_impl() = default;

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

template<typename T>
class CellAtomicModel: public CellAtomicModel_impl
{
public:
	typedef T t_type;

public:
	CellAtomicModel(std::string name, t_point pt, const T& value, std::size_t priority = 0):
		CellAtomicModel_impl(name, pt, priority)
	{
		initState(n_tools::createRawObject<State__impl<t_type>>(value));
	}
	CellAtomicModel(std::string name, t_point pt, const T& value, int coreNum, std::size_t priority = 0):
		CellAtomicModel_impl(name, pt, coreNum, priority)
	{
		initState(n_tools::createRawObject<State__impl<t_type>>(value));
	}
	CellAtomicModel(std::string name, t_point pt, std::size_t priority = 0):
		CellAtomicModel_impl(name, pt, priority)
	{
		initState(n_tools::createRawObject<State__impl<t_type>>());
	}
	CellAtomicModel(std::string name, t_point pt, int coreNum, std::size_t priority = 0):
		CellAtomicModel_impl(name, pt, coreNum, priority)
	{
		initState(n_tools::createRawObject<State__impl<t_type>>());
	}

	/**
	 * @brief Returns a reference to the current state.
	 */
	constexpr const t_type& state() const
	{
            return n_tools::staticRawCast<State__impl<t_type>>(getState())->m_value;
	}

	/**
	 * @brief Returns a reference to the current state.
	 */
	t_type& state()
	{
	        return n_tools::staticRawCast<State__impl<t_type>>(getState())->m_value;
	}

};

typedef std::shared_ptr<CellAtomicModel_impl> t_cellmodelptr;

} /* namespace n_model */

#endif /* SRC_MODEL_CELLMODEL_H_ */
