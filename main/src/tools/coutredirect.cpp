/*
 * coutredirect.cpp
 *
 *  Created on: Apr 1, 2015
 *      Author: Stijn Manhaeve - Devs Ex Machina
 */

#include "tools/coutredirect.h"
#include <cassert>

namespace n_tools {

CoutRedirect::CoutRedirect(std::streambuf* newBuf)
{
	assert(newBuf && "CoutRedirect::CoutRedirect new buffer can't be a nullptr");
	std::cout.flush();
	m_oldBuf = std::cout.rdbuf();
	std::cout.rdbuf(newBuf);
}

CoutRedirect::CoutRedirect(std::ostream& sourceStream): CoutRedirect(sourceStream.rdbuf())
{
}

CoutRedirect::~CoutRedirect()
{
	std::cout.flush();
	std::cout.rdbuf(m_oldBuf);
}

} /* namespace n_tools */
