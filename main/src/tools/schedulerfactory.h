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
	FIBONACCI, BINOMIAL
};

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
