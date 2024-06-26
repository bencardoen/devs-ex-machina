/*
 * This file is part of the DEVS Ex Machina project.
 * Copyright 2014 - 2015 University of Antwerp
 * https://www.uantwerpen.be/en/
 * Licensed under the EUPL V.1.1
 * A full copy of the license is in COPYING.txt, or can be found at
 * https://joinup.ec.europa.eu/community/eupl/og_page/eupl
 *      Author: Ben Cardoen
 */

#ifndef STLSCHEDULER_H
#define STLSCHEDULER_H

#include "scheduler/scheduler.h"
#include <vector>
#include <deque>
#include <queue>
#include <map>
#include <sstream>

namespace n_scheduler {

// Forward declare friend
template<typename T>
class SchedulerFactory;

/**
 */
template<typename S>
class STLScheduler: public Scheduler<S> {

private:

	friend class SchedulerFactory<S> ;
        
        std::priority_queue<S,std::deque<S>>  m_storage;

public:
	STLScheduler() {
		;
	}

public:

	virtual ~STLScheduler() {
		;
	}

	/**
	 * @brief push Schedule item.
	 * @pre S has a fully ordered implementation of S::operator< or std::less<S>
	 * @post size += 1
	 */
	virtual void push_back(const S&) override;

	virtual size_t size() const override;

	/**
	 * @brief empty
	 * @attention : if you use empty to protect from
	 * popping empty scheduler, make sure both empty() and pop()
	 * are wrapped in a single lock.
	 * @return
	 */
	virtual bool empty() const override;

	virtual const S& top() override;

	virtual S pop() override;

	virtual bool isLockable() const override;

	virtual
	void
	unschedule_until(std::vector<S> &container, const S& elem) override;

	void
	clear() override;

	virtual
	bool
	contains(const S& elem) const override;

	virtual
	bool
	erase(const S& elem) override;

	virtual
	void
	printScheduler() const override;

	virtual
	void
	testInvariant() const override;

};

}
#include "stlscheduler.tpp"
#endif
