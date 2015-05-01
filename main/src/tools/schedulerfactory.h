/**
 * @author Ben Cardoen.
 */

#include "scheduler.h"

#ifndef SCHEDULERFACTORY_H
#define SCHEDULERFACTORY_H

namespace n_tools {

/**
 * @brief The Storage enum Defines a user preference for the storage type the
 * scheduler will use. This makes it somewhat easier to use, and does not
 * expose the template storage parameter.
 */
enum class Storage
{
	/** Node based, with amortized complexity for push_back(), can be faster than binomial.*/
	FIBONACCI,
	/** Node based, O(log n) for all operations.*/
	BINOMIAL,
	/** Complexity is not yet proven, but 'tends' to be faster. Node-based. */
	PAIRING,
	/** Self adjusting node based heap. Fast merge (not exposed). */
	SKEW,
	/** Arity value determines nr of children in non-leaf node. Uses list
	 *  as internal storage in contrast to node based heaps.
	 *  @ATTENTION: do not use with -D_GLIBCXX_DEBUG, triggers a false eror on safe_iterator. (verified with valgrind)
	 */
	D_ARY
};

/*! \var Test::TEnum Test::Val1
 * The description of the first enum value.
 */


/**
 * Provides easy creation to Scheduler instances.
 * @example
 * auto scheduler = SchedulerFactory<int>::makeScheduler(Storage::FIBONACCI);
 * scheduler is of type std::shared_ptr<Scheduler<int>>;
 */
template<typename X>
class SchedulerFactory
{
private:
	SchedulerFactory() = delete;
public:
	typedef std::shared_ptr<Scheduler<X>> t_Scheduler;

	static std::shared_ptr<Scheduler<X>>
	makeScheduler(const Storage&, bool synchronized = false);
};


}
#include "schedulerfactory.tpp"

#endif // SCHEDULERFACTORY_H
