/*
 * timestamp.h
 *
 *  Created on: 9 Mar 2015
 *      Author: Ben Cardoen
 */

#ifndef SRC_NETWORK_TIMESTAMP_H_
#define SRC_NETWORK_TIMESTAMP_H_

#include <chrono>
#include <mutex>
#include <ctime>
#include <type_traits>
#include <sstream>

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
	const T m_timestamp;

	inline
	bool timeStampsEqual(const T& lhs, const T& rhs) const
	{
		return (lhs.m_timestamp == rhs.m_timestamp);
	}

	/**
	 * Defines 'happens before' (as in Lamport clocks).
	 * A.m_causal < B.m_causal IF A happened before B (and time field is equal)
	 */
	const X m_causal;
public:
	typedef T t_time;
	typedef X t_causal;

	Time() = delete;
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

	friend std::ostream&
	operator<<(std::ostream& os, const Time& t)
	{
		os << "TimeStamp ::" << t.getTime();
		if (t.m_causal != 0) {
			os << "\tcausal ::" << t.m_causal;
		}
		return os << std::endl;
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
};

// In practice, use this typedef.
typedef Time<std::size_t, std::size_t> TimeStamp;

/**
 * Convenience function : make a TimeStamp object reflecting the current time.
 */
TimeStamp makeTimeStamp(size_t causal = 0)
{
	static std::mutex lock;
	std::lock_guard<std::mutex> locknow(lock);
	TimeStamp::t_time now = std::chrono::high_resolution_clock::now().time_since_epoch().count();
	return TimeStamp(now, causal);
}

/**
 * Given a Timestamp, make another with identical time field, but happening after the
 * original.
 */
TimeStamp makeCausalTimeStamp(const TimeStamp& before)
{
	TimeStamp after(before.getTime(), before.getCausality() + 1);
	return after;
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
