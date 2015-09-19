/*
 * objectFactory.h
 *
 *  Created on: 7 Mar 2015
 *      Author: Ben Cardoen -- Stijn Manhaeve
 *      Using the following url as help : http://eli.thegreenplace.net/2014/variadic-templates-in-c/
 */
#include <memory>

#ifndef SRC_TOOLS_OBJECTFACTORY_H_
#define SRC_TOOLS_OBJECTFACTORY_H_

namespace n_tools {

/**
 * Create shared_ptr to the new Object with constructor arguments Args.
 */
template<typename T, typename ... Args>
std::shared_ptr<T> createObject(Args&&... args)
{
	return std::make_shared<T>(std::forward<Args>(args)...);
	//return std::allocate_shared<T>(boost::pool_allocator<T>(), std::forward<Args>(args)...);
}

/**
 * Create a raw pointer to the new Object with constructor arguments Args.
 * @warning	atm, you have to make sure you delete the object yourself.
 * 		This may change in the future so be warned!
 */
template<typename T, typename ... Args>
T* createRawObject(Args&&... args){
	return new T(args...);
}

/**
 * Takes back a pointer created by createRawObject and clears its memory
 */
template<typename T>
void takeBack(T* pointer){
	delete pointer;
}

#if USE_SAFE_CAST
template<class T>
struct castReturn
{
	typedef std::shared_ptr<T> t_type;
};

template<typename to, typename from>
inline typename castReturn<to>::t_type staticCast(const from& value)
{
	return std::static_pointer_cast<to>(value);
}

template<typename to, typename from>
inline typename castReturn<to>::t_type dynamicCast(const from& value)
{
	return std::dynamic_pointer_cast<to>(value);
}
#else /* USE_SAFE_CAST */
template<class T>
struct castReturn
{
	typedef T* t_type;
};

/**
 * @brief custom implementation of static_pointer_cast
 */
template<typename to, typename from>
inline typename castReturn<to>::t_type staticCast(const from& value)
{
	return reinterpret_cast<typename castReturn<to>::t_type>(value.get());
}

template<typename to, typename from>
inline typename castReturn<to>::t_type dynamicCast(const from& value)
{
	return dynamic_cast<typename castReturn<to>::t_type>(value.get());
}
#endif /* USE_SAFE_CAST */
} /* namespace n_tools */

#endif /* SRC_TOOLS_OBJECTFACTORY_H_ */
