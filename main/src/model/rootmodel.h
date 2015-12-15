/*
 * This file is part of the DEVS Ex Machina project.
 * Copyright 2014 - 2015 University of Antwerp 
 * https://www.uantwerpen.be/en/
 * Licensed under the EUPL V.1.1
 * A full copy of the license is in COPYING.txt, or can be found at 
 * https://joinup.ec.europa.eu/community/eupl/og_page/eupl 
 *      Author: Stijn Manhaeve, Ben Cardoen, Tim Tuijn, Matthijs Van Os
 */

#ifndef ROOTMODEL_H_
#define ROOTMODEL_H_

#include "model/coupledmodel.h"
#include "model/atomicmodel.h"
#include "tools/globallog.h"

namespace n_model {
class RootModel final	//don't allow users to derive from this class
{
private:
	std::vector<t_atomicmodelptr> m_components;
	bool m_directConnected;

public:
	RootModel();
	void setComponents(const t_coupledmodelptr& model);
	/**
	 * @precondition All atomic models have a unique name
	 */
	const std::vector<t_atomicmodelptr>& directConnect(const t_coupledmodelptr&);
	void undoDirectConnect()
	{ m_directConnected = false;}

	const std::vector<t_atomicmodelptr>& getComponents()const
	{ return m_components;}
	std::vector<t_atomicmodelptr>& getComponents()
	{ return m_components;}

	void reset();
};
} /* namespace n_model */

#endif /* ROOTMODEL_H_ */
