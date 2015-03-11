/*
 * LocationTable.h
 *
 *  Created on: 11 Mar 2015
 *      Author: matthijs
 */

#ifndef SRC_CONTROL_LOCATIONTABLE_H_
#define SRC_CONTROL_LOCATIONTABLE_H_

namespace n_control {

class LocationTable
{
public:
	LocationTable(uint amountCores);
	virtual ~LocationTable();

	CoreID lookupModel(std::string modelName);
	void registerModel(const Model& model);

private:
	uint m_amountCores;
};

} /* namespace n_control */

#endif /* SRC_CONTROL_LOCATIONTABLE_H_ */
