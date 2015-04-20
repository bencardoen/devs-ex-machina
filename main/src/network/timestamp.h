/*
 * timestamp.h
 *
 *  Created on: 9 Mar 2015
 *      Author: Ben Cardoen
 */

#ifndef SRC_NETWORK_TIMESTAMP_H_
#define SRC_NETWORK_TIMESTAMP_H_

#include "archive.h"
#include <chrono>
#include <mutex>
#include <cmath>
#include <ctime>
#include <type_traits>
#include <sstream>
#include <cmath>
#include <memory>

namespace n_network {

// Declare Comparison for integral, floating point types using SFINAE && enable_if

template<class T, typename std::enable_if<std::is_floating_point<T>::value>::type* = nullptr>
bool nearly_equal(const T& left, const T& right)
{
	// Use an epsilon value of approx 2e-12 (not perfect).
	static constexpr T eps = std::numeric_limits<T>::epsilon() * 1000;
	return (std::fabs(left - right) < eps);
}

template<class T, typename std::enable_if<std::is_integral<T>::value>::type* = nullptr>
bool nearly_equal(const T& left, const T& right)
{
	return (left == right);
}

/**
 * Represents a timestamp with optional causal ordering.
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

	inline
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

	static inline Time infinity()
	{
		return Time(std::numeric_limits<T>::max(), std::numeric_limits<X>::max());
	}

	Time(): m_timestamp(0), m_causal(0){}// = default;
	Time(t_time time, t_causal causal = 0)
		: m_timestamp(time), m_causal(causal)
	{
		;
	}
	virtual ~Time()
	{
		;
	}
	// Assignment, move and copy are correct for POD.

	t_time getTime() const
	{
		return this->m_timestamp;
	}

	t_causal getCausality() const
	{
		return this->m_causal;
	}

	/**
	 * Simulates CPDevs' select function. A priority/offset of 0 (highest) will force a model to be chosen first.
	 */
	void increaseCausality(const X& offset)
	{
		this->m_causal += offset;
	}

	friend std::ostream&
	operator<<(std::ostream& os, const Time& t)
	{
		if(t == infinity()){
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
	bool operator<(const Time& lhs, const Time& rhs)
	{
		if (nearly_equal<t_time>(lhs.getTime(), rhs.getTime()))
			return lhs.m_causal < rhs.m_causal;
		else
			return lhs.m_timestamp < rhs.m_timestamp;
	}

	friend
	bool operator>(const Time& lhs, const Time& rhs)
	{
		// a > b implies b < a
		return (rhs < lhs);
	}

	friend
	bool operator>=(const Time& lhs, const Time& rhs)
	{
		// a >= b implies not(a<b)
		return (not (lhs < rhs));
	}

	friend
	bool operator<=(const Time&lhs, const Time&rhs)
	{
		// a <= b  implies (not a>b)
		return (not (lhs > rhs));
	}

	friend
	bool operator==(const Time& lhs, const Time& rhs)
	{
		return (nearly_equal<t_time>(lhs.getTime(), rhs.getTime()) && lhs.m_causal == rhs.m_causal);
	}

	friend
	bool operator!=(const Time& lhs, const Time& rhs)
	{
		return (not (lhs == rhs));
	}

	/**
	 * Addition of time object
	 * @return (Time (left+right), max(left,right))
	 */
	friend Time operator+(const Time& lhs, const Time& rhs)
	{
		if(rhs == infinity())
			return infinity();
		return Time(lhs.m_timestamp + rhs.m_timestamp, std::max(lhs.m_causal, rhs.m_causal));
	}

	/**
	 * Serialize this object to the given archive
	 *
	 * @param archive A container for the desired output stream
	 */
	void serialize(n_serialisation::t_oarchive& archive)
	{
		archive(m_timestamp, m_causal);
	}

	/**
	 * Unserialize this object to the given archive
	 *
	 * @param archive A container for the desired input stream
	 */
	void serialize(n_serialisation::t_iarchive& archive)
	{
		archive(m_timestamp, m_causal);
	}
};

// In practice, use this typedef.
typedef Time<std::size_t, std::size_t> t_timestamp;

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
inline t_timestamp makeCausalTimeStamp(const t_timestamp& before)
{
	t_timestamp after(before.getTime(), before.getCausality() + 1);
	return after;
}

inline t_timestamp makeLatest(const t_timestamp& now)
{
	return t_timestamp(now.getTime(), std::numeric_limits<t_timestamp::t_causal>::max());
}

} /* namespace n_network */

namespace std {
template<typename T, typename X>
struct hash<n_network::Time<T, X>>
{
	size_t operator()(const n_network::Time<T, X>& item) const
	{
		constexpr int prime = 17;
		// Could left shift, but what if time is Floating point ??
		// todo use enable_if floating point to get better results here.
		// @warning : test with -fsanitize=integer (mind overflow).
		size_t result = hash<T>()(item.getTime()) * prime;
		result += hash<X>()(item.getCausality()) * prime;	// second field is very small.
		return result;
	}
};
}

#endif /* SRC_NETWORK_TIMESTAMP_H_ */
