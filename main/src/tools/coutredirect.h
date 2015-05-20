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

/**
 * @brief Redirects the output written to std::cout to a different sink.
 * @warning Always make sure that, if you use multiple instances simultaneously,
 * 		these instances are destructed in the reverse order in which they are constructed.
 */
class CoutRedirect
{
private:
	std::streambuf* m_oldBuf;
public:
	/**
	 * @brief Constructor with a stream buffer object.
	 * @param newBuf The new stream buffer that will be used to buffer output for std::cout
	 */
	CoutRedirect(std::streambuf* newBuf);
	/**
	 * @brief Constructor with a different output stream
	 * @param sourceStream
	 */
	CoutRedirect(std::ostream& sourceStream);

	/**
	 * @brief Destructor. Will reset std::cout so that it uses its former output buffer again.
	 */
	~CoutRedirect();

};

} /* namespace n_tools */

#endif /* SRC_TOOLS_COUTREDIRECT_H_ */
