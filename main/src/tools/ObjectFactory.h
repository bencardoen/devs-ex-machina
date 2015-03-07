/*
 * ObjectFactory.h
 *
 *  Created on: 7 Mar 2015
 *      Author: Ben Cardoen
 *      Using the following url as help : http://eli.thegreenplace.net/2014/variadic-templates-in-c/
 */
#include <memory>

#ifndef SRC_TOOLS_OBJECTFACTORY_H_
#define SRC_TOOLS_OBJECTFACTORY_H_

namespace n_tools {

/**
 * Create shared_ptr to the new Object with constructor arguments Args.
 */
template<typename T, typename... Args>
std::shared_ptr<T> createObject(Args&&... args)
{
	// Note to self: std::forward keeps (rlx)valuetype intact.
	// If needed, replace with allocate_shared...
	return std::make_shared<T>(std::forward<Args>(args)...);
}
} /* namespace n_tools */

#endif /* SRC_TOOLS_OBJECTFACTORY_H_ */
