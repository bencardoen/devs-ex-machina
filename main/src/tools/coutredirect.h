/*
 * coutredirect.h
 *
 *  Created on: Apr 1, 2015
 *      Author: Stijn Manhaeve - Devs Ex Machina
 */

#ifndef SRC_TOOLS_COUTREDIRECT_H_
#define SRC_TOOLS_COUTREDIRECT_H_

#include <streambuf>
#include <iostream>
#include <fstream>

namespace n_tools {

class CoutRedirect
{
private:
	std::streambuf* m_oldBuf;
public:
	CoutRedirect(std::streambuf* newBuf);
	CoutRedirect(std::ostream& sourceStream);
	~CoutRedirect();

};

} /* namespace n_tools */

#endif /* SRC_TOOLS_COUTREDIRECT_H_ */
