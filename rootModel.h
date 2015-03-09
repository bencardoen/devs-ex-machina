/*
 * rootModel.h
 *
 *  Created on: 9-mrt.-2015
 *      Author: Pieter
 */

#ifndef ROOTMODEL_H_
#define ROOTMODEL_H_

#include "atomicModel.h"

namespace model {
	class RootModel {
	private:
		std::vector<AtomicModel> m_components;
		bool m_directConnected;

	public:
		void directConnect();
		void undoDirectConnect();
		void setGVT(t_time gvt);
		void revert(t_time oldTime);
	};
}

#endif /* ROOTMODEL_H_ */
