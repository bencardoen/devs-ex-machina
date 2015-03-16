/*
 * Configuration.cpp
 *
 *  Created on: 11 Mar 2015
 *      Author: matthijs
 */

#include "configuration.h"

namespace n_control {

Configuration::Configuration(Formalism formalism)
	: m_formalism(formalism)
{
}

Configuration::~Configuration()
{
}

Formalism Configuration::getFormalism()
{
	return m_formalism;
}

} /* namespace n_control */
