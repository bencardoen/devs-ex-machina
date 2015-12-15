/*
 * This file is part of the DEVS Ex Machina project.
 * Copyright 2014 - 2015 University of Antwerp
 * https://www.uantwerpen.be/en/
 * Licensed under the EUPL V.1.1
 * A full copy of the license is in COPYING.txt, or can be found at
 * https://joinup.ec.europa.eu/community/eupl/og_page/eupl
 *      Author: Stijn Manhaeve, Tim Tuijn
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
