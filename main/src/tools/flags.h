/*

 * flags.h
 *	Basic functions for working with bit flags, used in threadsignalling.
 * 	Created on: 30 Apr 2015
 *	Author: Ben Cardoen
 */

#ifndef SRC_TOOLS_FLAGS_H_
#define SRC_TOOLS_FLAGS_H_

namespace n_tools{

/** Cannot partially specialize, so only provide template for size_t (argument type).
 *  These templates (try to) protect against mixing 32/64 bit unsigned ints into the compiler
 *  intrinsics.
 *  Return position of first set bit (MSB->LSB) in param, or sizeof(param)*8 if param = 0.
 *  Use the convenience function pos_first_one, not the template functions.
 */
template<size_t>
size_t firstbitsetimp(size_t);

// Specialization for 32 bit unsigned using compiler intrinsics. This should reduce to asm bsr on intel.
template<>
inline
size_t firstbitsetimp<4>(size_t i)
{
        return __builtin_clz(i);
}

// Specialization for 64 bit unsigned.
template<>
inline
size_t firstbitsetimp<8>(size_t i)
{
        return __builtin_clzl(i);
}

/**
 * Returns the position (MSB->LSB) of the first one in param, or sizeof(param)*8 if param==0.
 */
inline
size_t pos_first_one(size_t i)
{
        return firstbitsetimp<sizeof(i)>(i);
}

/**
 * Naive O(N) detection of firstbitset.
 * Rightshift param until 0.
 * @param ex : 0010 -> [0]001 -> [00]01 -> [000]0. 3 shifts, size-nr shifts = 1
 * @param i The bit index of the first 1 (MSB->LSB) if i != 0, else sizeof(i)
 * @return 
 */
inline
size_t fbsnaive(size_t i)
{
        size_t shiftcount = 0;
        while(i){
                i = i >> 1ull;
                ++shiftcount;
        }
        return (sizeof(i)*8)-shiftcount;             
}

/**
 * @pre flag is power of 2
 */
template<typename T=std::size_t>
constexpr bool flag_is_set(const T& testvalue, const T& flag){
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
