/*
 * tracers.h
 *
 *  Created on: Mar 12, 2015
 *      Author: Stijn Manhaeve - Devs Ex Machina
 */

#ifndef SRC_TRACING_TRACERS_H_
#define SRC_TRACING_TRACERS_H_

#include <tuple>
#include "network/timestamp.h"

using namespace n_network;

namespace n_tracers {

//tracer class
//base case; no tracers, still needs the variadic template parameters because of reasons.
/**
 * @brief Tracer container class.
 * @tparam TracerElems... Ordered list of tracer types.
 *
 * This class defines an ordered list of tracer objects.
 * It will send the trace requests to all registered tracers.
 * Because this implementation uses compile time polymorphism,
 * it can be faster than the naive implementation that uses runtime polymorphism,
 * if optimized correctly.
 */
template<typename ... TracerElems> class Tracers
{
public:
	/**
	 * @brief Default constructor. Creates an empty Tracers collection.
	 */
	Tracers()
	{
	}

	/**
	 * @brief Returns the amount of registered tracers.
	 */
	std::size_t getSize() const{
		return 0;
	}

	/**
	 * @brief Returns whether any tracers are registered.
	 * @note Disabled tracers still count.
	 */
	bool hasTracers() const{
		return false;
	}

	/**
	 * @brief getTracer overload for when the argument is out of bounds.
	 * The compiler error will complain that a deleted function has been invoked,
	 * pointing to the place where the function is actually invoked.
	 */
	template<std::size_t n>
	const void*
	getByID() const = delete;


	/**
	 * @brief getTracer overload for when the argument is out of bounds.
	 * The compiler error will complain that a deleted function has been invoked,
	 * pointing to the place where the function is actually invoked.
	 */
	template<std::size_t n>
	void*
	getByID() = delete;

	/**
	 * @brief Traces internal state transition
	 * @param time The timestamp of the transition (temporary for testing)
	 *
	 * @note Because this is just a quick prototype, the function doesn't actually trace anything
	 * @TODO Change to actual trace function traceInternal(ADevs* devs)
	 */
	template<typename Scheduler>
	void tracesInternal(t_timestamp time, Scheduler* scheduler){
	}
};

//recursive case; take one type from the pack and continue with the rest.
//Because of the first template parameter,
//this implementation has higher precedence over the previous one
/**
 * @brief Fully templated tracer container class.
 * @tparam TracerElems... Ordered list of tracer types.
 *
 * This class defines an ordered list of tracer objects.
 * It will send the trace requests to all registered tracers.
 * Because this implementation uses compile time polymorphism,
 * it can be faster than the naive implementation that uses runtime polymorphism,
 * if optimized correctly.
 */
template<typename T, typename ... TracerElems> class Tracers<T, TracerElems...> : public Tracers<TracerElems...>
{
public:
	/**
	 * @brief Constructor
	 * @param tracer... Tracers to be put into this collection
	 * This constructor allows creating and configuring the individual tracers before putting them into the collection of tracers.
	 * @precondition All tracer classes must be copyable.
	 */
	Tracers(T&& tracer, TracerElems&& ... other)
		: Tracers<TracerElems...>(other...), m_elem(tracer)
	{
	}

	/**
	 * @brief Default constructor
	 * All tracers are created with their default constructor.
	 */
	Tracers(): Tracers<TracerElems...>(), m_elem()
	{
	}
//	TracersTemplated(T tracer, TracerElems ... other)
//		: TracersTemplated<TracerElems...>(other...), elem(tracer)
//	{
//	}

	/*
	 * implementation of the constructor that uses move semantics.
	 * I would love to be able to use both of them automatically.
	 */
//	TracersTemplated(T&& tracer, TracerElems&& ... other)
//			: TracersTemplated<TracerElems...>(std::forward<TracerElems>(other)...), elem(std::forward<T>(tracer))
//	{
//	}


	/**
	 * @brief Returns the amount of registered tracers.
	 */
	std::size_t getSize() const{
		return (sizeof...(TracerElems) + 1);
	}

	/**
	 * @brief Returns whether any tracers are registered.
	 * @note Disabled tracers still count.
	 */
	bool hasTracers() const{
		return true;
	}

	/**
	 * @brief Gets a const reference to the first tracer in line.
	 * @return The very first tracer in this collection of tracers.
	 */
	const T& getTracer() const{
		return m_elem;
	}

	/**
	 * @brief Gets a reference to the first tracer in line.
	 * @return The very first tracer in this collection of tracers.
	 */
	T& getTracer(){
		return m_elem;
	}

	/*
	 * explanation of why getTracer works:
	 * ===================================
	 * The implementation of getTracer is based on the SFINAE idiom
	 *  - Substitution Failure Is Not An Error (http://en.wikipedia.org/wiki/Substitution_failure_is_not_an_error)
	 * Basically, when calling the template function, the compiler first tries
	 * the first version of getTracer. If this results in an error, it tries again with the next implementation.
	 * The implementations only differ in their return type:
	 * typename std::enable_if<n!=0,
	 * 	typename std::tuple_element<n, std::tuple<T, TracerElems...>>::type>::type
	 *
	 * The inner typename is the type of the nth registered tracer
	 * 	std::tuple_element<n, std::tuple<T, TracerElems...>>::type
	 * The outer typenam is defined as follows:
	 * 	std::enable_if<condition, A>::type
	 * If condition evaluates to true, the struct std::enable_if defines a typedef type with value A
	 * If condition evaluates to false however, the struct does not define a type at all, resulting in a compiler error.
	 */
	/**
	 * @brief gets a reference to nth tracer in line, counting from 0.
	 * @tparam n The number of the tracer that will be returned.
	 * @precondition [static] n must be in range. e.g. if 5 tracers were specified, n must be smaller than 5.
	 */
	template<std::size_t n>
	typename std::enable_if<n <= sizeof...(TracerElems) && n != 0,
		typename std::tuple_element<n, std::tuple<T, TracerElems...>>::type>::type&
	getByID(){
		static_assert(n <= sizeof...(TracerElems), "TracersTemplated::getByID n out of bounds.");
		//the assertion well never be called though. I added it for extra clarity
        return getNext(). template getByID<n-1>();
	}

	/**
	 * @brief gets a reference to nth tracer in line, counting from 0.
	 * @tparam n The number of the tracer that will be returned.
	 * @precondition [static] n must be in range. e.g. if 5 tracers were specified, n must be smaller than 5.
	 */
	template<std::size_t n>
	typename std::enable_if<n==0 ,
		typename std::tuple_element<n, std::tuple<T, TracerElems...>>::type>::type&
	getByID(){
		static_assert(n <= sizeof...(TracerElems), "TracersTemplated::getByID n out of bounds.");
		//the assertion well never be called though. I added it for extra clarity
		return getTracer();
	}

	/**
	 * @brief gets a const reference to nth tracer in line, counting from 0.
	 * @tparam n The number of the tracer that will be returned.
	 * @precondition [static] n must be in range. e.g. if 5 tracers were specified, n must be smaller than 5.
	 */
	template<std::size_t n>
	const typename std::enable_if<n <= sizeof...(TracerElems) && n != 0,
		typename std::tuple_element<n, std::tuple<T, TracerElems...>>::type>::type&
	getByID() const{
		static_assert(n <= sizeof...(TracerElems), "TracersTemplated::getByID n out of bounds.");
		//the assertion well never be called though. I added it for extra clarity
        return getNext(). template getByID<n-1>();
	}

	/**
	 * @brief gets a const reference to nth tracer in line, counting from 0.
	 * @tparam n The number of the tracer that will be returned.
	 * @precondition [static] n must be in range. e.g. if 5 tracers were specified, n must be smaller than 5.
	 */
	template<std::size_t n>
	const typename std::enable_if<n==0,
		typename std::tuple_element<n, std::tuple<T, TracerElems...>>::type>::type&
	getByID() const{
		static_assert(n <= sizeof...(TracerElems), "TracersTemplated::getTracer n out of bounds.");
		//the assertion well never be called though. I added it for extra clarity
		return getTracer();
	}

	/**
	 * @brief getTracer overload for when the argument is out of bounds.
	 * The compiler error will complain that a deleted function has been invoked,
	 * pointing to the place where the function is actually invoked.
	 */
	template<std::size_t n>
	const typename std::enable_if<sizeof...(TracerElems) < n, void*>::type
	getByID() const = delete;


	/**
	 * @brief getTracer overload for when the argument is out of bounds.
	 * The compiler error will complain that a deleted function has been invoked,
	 * pointing to the place where the function is actually invoked.
	 */
	template<std::size_t n>
	const typename std::enable_if<sizeof...(TracerElems) < n, void*>::type
	getByID() = delete;

	/**
	 * @brief Traces internal state transition
	 * @param time The timestamp of the transition (temporary for testing)
	 *
	 * @note Because this is just a quick prototype, the function doesn't actually trace anything
	 * @TODO Change to actual trace function traceInternal(ADevs* devs)
	 */
	template<typename Scheduler>
	void tracesInternal(t_timestamp time, Scheduler* scheduler){
		m_elem.traceInternal(time, scheduler);
		getNext().traceInternal(time, scheduler);
	}

private:
	T m_elem;

	/**
	 * @brief retrieves a reference to the sliced instance of the direct superclass
	 */
	Tracers<TracerElems...>& getNext(){
		return *static_cast<Tracers<TracerElems...>*>(this);
	}
	/**
	 * @brief retrieves a const reference to the sliced instance of the direct superclass
	 */
	const Tracers<TracerElems...>& getNext() const{
		return *static_cast<Tracers<TracerElems...>*>(this);
	}
};

} /*namespace n_tracers*/

#endif /* SRC_TRACING_TRACERS_H_ */
