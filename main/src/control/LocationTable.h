/*
 * LocationTable.h
 *
 *  Created on: 11 Mar 2015
 *      Author: matthijs
 */

#ifndef SRC_CONTROL_LOCATIONTABLE_H_
#define SRC_CONTROL_LOCATIONTABLE_H_

#include "Controller.h"

//TEMPORARY:
//FIXME
namespace n_core {
typedef int t_coreID;
} /* namespace n_core */

namespace n_control {

class LocationTable
{
	typedef std::shared_ptr<n_model::Model> t_modelPtr;
public:
	LocationTable(uint amountCores);
	virtual ~LocationTable();

	n_core::t_coreID lookupModel(std::string modelName);
	void registerModel(t_modelPtr model);

private:
	uint m_amountCores;
	std::unordered_map<std::string, n_core::t_coreID> m_locTable;
};

} /* namespace n_control */

#endif /* SRC_CONTROL_LOCATIONTABLE_H_ */
