/*
 * This file is part of the DEVS Ex Machina project.
 * Copyright 2014 - 2015 University of Antwerp
 * https://www.uantwerpen.be/en/
 * Licensed under the EUPL V.1.1
 * A full copy of the license is in COPYING.txt, or can be found at
 * https://joinup.ec.europa.eu/community/eupl/og_page/eupl
 *      Author: Ben Cardoen, Stijn Manhaeve
 */

#include <memory>
#include "tools/globallog.h"
#include "pools/pools.h"

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
T* createRawObject(Args&&... args)
{
	return new T(args...);
}

/**
 * Takes back a pointer created by createRawObject and clears its memory
 */
template<typename T>
void takeBack(T* pointer)
{
	delete pointer;
}

/**
 * Create an object from a pool.
 * @param args (optionally empty) argument list for (any) constructor of type T. 
 */
template<typename T, typename ... Args>
T* createPooledObject(Args&&... args)
{
        T* mem = n_pools::getPool<T>()->allocate();     // Calling thread gets a dedicated pool.
        T* obj = new (mem) T(args...);
        LOG_DEBUG("Allocating pooled object : ", obj);
        return obj;
}

/**
 * Call only iff runtime type of t == T.
 * @return Destructor of t is invoked, object pointed to by t is no longer valid.
 */
template<typename T>
void destroyPooledObject(T* t)
{
        LOG_DEBUG("Deallocating pooled object : ", t);
        n_pools::getPool<T>()->deallocate(t);
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

template<class T>
struct castRawReturn
{
	typedef T* t_type;
};

/**
 * @brief custom implementation of static_pointer_cast
 */
template<typename to, typename from>
inline typename castRawReturn<to>::t_type staticRawCast(const from& value)
{
	return reinterpret_cast<typename castRawReturn<to>::t_type>(value);
}

template<typename to, typename from>
inline typename castRawReturn<to>::t_type dynamicRawCast(const from& value)
{
	return dynamic_cast<typename castRawReturn<to>::t_type>(value);
}

} /* namespace n_tools */

#endif /* SRC_TOOLS_OBJECTFACTORY_H_ */
