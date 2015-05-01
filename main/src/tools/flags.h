/*

 * flags.h
 *	Basic functions for working with bit flags, used in threadsignalling.
 * 	Created on: 30 Apr 2015
 *	Author: Ben Cardoen
 */

#ifndef SRC_TOOLS_FLAGS_H_
#define SRC_TOOLS_FLAGS_H_

namespace n_tools{

/**
 * @pre flag is power of 2
 */
template<typename T=std::size_t>
bool flag_is_set(const T& testvalue, const T& flag){
	return ((testvalue & flag)==flag);
}

/**
 * @pre flag is power of 2
 */
template<typename T=std::size_t>
void unset_flag(T& testvalue, const T& flag){
	testvalue &= ~flag;
}

/**
 * @pre flag is power of 2
 */
template<typename T=std::size_t>
void set_flag(T& testvalue, const T& flag){
	testvalue |= flag;
}

}

/**
 * Keeps flags used in intra-thread signalling.
 * @see cvworker
 */
namespace n_threadflags{
/**
 * A thread should not wait but can simulate without restriction.
 * Conversely, unset this flag to indicate that a thread should queue into the
 * provided condition variable and wait until released.
 * ~SHOULDWAIT
 */
constexpr std::size_t FREE = 1;		// ~SHOULDWAIT

/**
 * Denote that the simulating Core has reached termination time, and is no longer advancing. This can be
 * a temporary state (a revert can undo this state). It is required to set this flag however to allow others threads to check
 * if everyone is/has finished simulating. Only then is a thread allowed to quit.
 */
constexpr std::size_t IDLE = 2;		// ~ISWORKING
/**
 * A thread sets this flag if it has detected that FREE is not set and it entered the condition variable.
 * This flag allows other threads to count how many (or all) other threads are waiting. For instance, a threadunsafe function
 * can be called after main verifies all threads are queued in the CVAR.
 */
constexpr std::size_t ISWAITING = 4;
/**
 * Upon detecting this flag, immediately halt execution (cleanly).
 */
constexpr std::size_t STOP = 8;
}
#endif /* SRC_TOOLS_FLAGS_H_ */
