/*
 * timestamp.h
 *
 *  Created on: 9 Mar 2015
 *      Author: Ben Cardoen
 */

#ifndef SRC_NETWORK_TIMESTAMP_H_
#define SRC_NETWORK_TIMESTAMP_H_

#include "serialization/archive.h"
#include <chrono>
#include <mutex>
#include <cmath>
#include <ctime>
#include <type_traits>
#include <sstream>
#include <cmath>
#include <memory>

namespace n_network {

template<class T, typename std::enable_if<std::is_floating_point<T>::value>::type* = nullptr>
bool nearly_equal(const T& left, const T& right)
{
	//static constexpr T eps = std::numeric_limits<T>::epsilon() * 1000;
        /* Epsilon double is 2.e-16, but only useful near [0,1]*/
        static constexpr T EPS = 2e-12;
	return (std::fabs(left - right) < EPS);
}

template<class T, typename std::enable_if<std::is_integral<T>::value>::type* = nullptr>
bool nearly_equal(const T& left, const T& right)
{
	return (left == right);
}

/**
 * Represents a timestamp with optional causal ordering.
 * Adevs' time is floating point based, this class can provide that as well by
 * setting template parameter T to double/float. Note that this severely complicates comparison.
 * Infinity is always defined as std::numeric_limits<Q>::max() for any type Q, so make sure this
 * specialization exist for your type.
 */
template<typename T, typename X>
class Time
{
private:
	/**
	 * Integral time in ticks, can be relative to epoch or 0.
	 * @see makeTimeStamp();
	 */
	T m_timestamp;

	constexpr
	bool timeStampsEqual(const T& lhs, const T& rhs) const
	{
		return (lhs.m_timestamp == rhs.m_timestamp);
	}

	/**
	 * Defines 'happens before' (as in Lamport clocks).
	 * A.m_causal < B.m_causal IF A happened before B (and time field is equal)
	 */
	X m_causal;
public:
	typedef T t_time;
	typedef X t_causal;

	static constexpr Time infinity()
	{
		return Time(std::numeric_limits<T>::max(), std::numeric_limits<X>::max());
	}

	constexpr Time(): m_timestamp(0), m_causal(0){}// = default;
	constexpr Time(t_time time, t_causal causal = 0)
		: m_timestamp(time), m_causal(causal)
	{
		;
	}
	// Assignment, move and copy are correct for POD.

	constexpr t_time getTime() const
	{
		return this->m_timestamp;
	}

	constexpr t_causal getCausality() const
	{
		return this->m_causal;
	}

	/**
	 * Simulates PDevs' select function. A priority/offset of 0 (highest) will force a model to be chosen first.
	 */
	void increaseCausality(const X& offset)
	{
		this->m_causal += offset;
	}

	friend std::ostream&
	operator<<(std::ostream& os, const Time& t)
	{
		if(isInfinity(t)){
			os << "inf";
			return os;
		}
		os << "TimeStamp ::" << t.getTime();
		if (t.m_causal != 0) {
			os << " causal ::" << t.m_causal;
		}
		return os;
	}

	friend
	constexpr bool operator<(const Time& lhs, const Time& rhs)
	{
		return (nearly_equal<t_time>(lhs.getTime(), rhs.getTime()))?
			(lhs.m_causal < rhs.m_causal) :
			(lhs.m_timestamp < rhs.m_timestamp);
	}

	friend
	constexpr bool operator>(const Time& lhs, const Time& rhs)
	{
		// a > b implies b < a
		return (rhs < lhs);
	}

	friend
	constexpr bool operator>=(const Time& lhs, const Time& rhs)
	{
		// a >= b implies not(a<b)
		return (not (lhs < rhs));
	}

	friend
	constexpr bool operator<=(const Time&lhs, const Time&rhs)
	{
		// a <= b  implies (not a>b)
		return (not (lhs > rhs));
	}

	friend
	constexpr bool operator==(const Time& lhs, const Time& rhs)
	{
		return (nearly_equal<t_time>(lhs.getTime(), rhs.getTime()) && lhs.m_causal == rhs.m_causal);
	}

	friend
	constexpr bool operator!=(const Time& lhs, const Time& rhs)
	{
		return (not (lhs == rhs));
	}

	/**
	 * Addition of time object
	 * @return (Time (left+right), max(left,right))
	 */
	friend
	constexpr Time operator+(const Time& lhs, const Time& rhs)
	{
		return (isInfinity(rhs) || isInfinity(lhs))?
			infinity() :
			Time(lhs.m_timestamp + rhs.m_timestamp, std::max(lhs.m_causal, rhs.m_causal));
	}

	/**
	 * Subtraction of time object
	 * @return (Time (left+right), max(left,right))
	 */
	friend
	constexpr Time operator-(const Time& lhs, const Time& rhs)
	{
		return (isInfinity(rhs) || isInfinity(lhs))?
			infinity():
			Time(lhs.m_timestamp - rhs.m_timestamp, std::min(lhs.m_causal, rhs.m_causal));
	}

	/**
	 * Serialize this object to the given archive
	 *
	 * @param archive A container for the desired output stream
	 */
	void serialize(n_serialization::t_oarchive& archive)
	{
		archive(m_timestamp, m_causal);
	}
        
        /**
         * Return true if the time part of the timestamp is infinite.
        * @attention not the same as ==infinity(), since that also checks causality.
        */
        friend
        constexpr bool isInfinity(const Time& arg){
                return(arg.getTime() == std::numeric_limits<Time::t_time>::max());
        }

	/**
	 * Unserialize this object to the given archive
	 *
	 * @param archive A container for the desired input stream
	 */
	void serialize(n_serialization::t_iarchive& archive)
	{
		archive(m_timestamp, m_causal);
	}
        
        static constexpr t_time MAXTIME = std::numeric_limits<Time::t_time>::max();
        static constexpr t_causal MAXCAUSAL = std::numeric_limits<Time::t_causal>::max();
};

} /* namespace n_network */
//load the typedef from a different file
#include "forwarddeclare/timestamp.h"

namespace n_network {

/**
 * Convenience function : make a TimeStamp object reflecting the current time.
 */
inline t_timestamp makeTimeStamp(size_t causal = 0)
{
	static std::mutex lock;
	std::lock_guard<std::mutex> locknow(lock);
	t_timestamp::t_time now = std::chrono::high_resolution_clock::now().time_since_epoch().count();
	return t_timestamp(now, causal);
}

/**
 * Given a t_timestamp, make another with identical time field, but happening after the
 * original.
 */
inline constexpr t_timestamp makeCausalTimeStamp(const t_timestamp& before)
{
	return t_timestamp(before.getTime(), before.getCausality() + 1);
}

inline constexpr t_timestamp makeLatest(const t_timestamp& now)
{
	return t_timestamp(now.getTime(), std::numeric_limits<t_timestamp::t_causal>::max());
}

} /* namespace n_network */

namespace std {
template<typename T, typename X>
struct hash<n_network::Time<T, X>>
{
	constexpr size_t operator()(const n_network::Time<T, X>& item) const
	{
#define prime 17
		// Could left shift, but what if time is Floating point ??
		// use enable_if floating point to get better results here.
		// @warning : test with -fsanitize=integer (mind overflow).
		// second field is very small.
		return hash<T>()(item.getTime()) * prime + hash<X>()(item.getCausality()) * prime;
#undef prime
	}
};
}

#endif /* SRC_NETWORK_TIMESTAMP_H_ */
