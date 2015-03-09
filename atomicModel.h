/*
 * atomicModel.h
 *
 *  Created on: 9-mrt.-2015
 *      Author: Pieter
 */

#ifndef ATOMICMODEL_H_
#define ATOMICMODEL_H_

#include "model.h"

namespace model {
	class AtomicModel {
	private:
		t_time m_elapsed;
		t_time m_lastRead;

	public:
		State extTransition(/*msg::Message message*/);
		State intTransition();
		State confTransition(/*msg::Message message*/);
		t_time timeAdvance();
		/*std::map<Port, msg::Message>*/ void output();
		void setGVT(t_time gvt);
		void revert(t_time time);
	};
}

#endif /* ATOMICMODEL_H_ */
