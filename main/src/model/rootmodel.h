/*
 * rootModel.h
 *
 *  Created on: 9-mrt.-2015
 *      Author: Pieter, Tim
 */

#ifndef ROOTMODEL_H_
#define ROOTMODEL_H_

#include "atomicmodel.h"

namespace n_model {
class RootModel : public Model
{
private:
	std::vector<AtomicModel> m_components;
	bool m_directConnected;

public:
	void directConnect();
	void undoDirectConnect();
	void setGVT(t_timestamp gvt);
	void revert(t_timestamp oldTime);
};
}

#endif /* ROOTMODEL_H_ */
