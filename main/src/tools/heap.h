/*
 * heap.h
 *
 *  Created on: Oct 3, 2015
 *      Author: lttlemoi
 */

#ifndef SRC_TOOLS_HEAP_H_
#define SRC_TOOLS_HEAP_H_

#include <algorithm>
#include "tools/globallog.h"

namespace n_tools {

/**
 * Performs the tricle up algorithm.
 * Returns true if at least one swap was made.
 */
template<typename RandomIterator, typename Compare>
    inline bool
    heap_tricleUp(RandomIterator first, RandomIterator last, RandomIterator item,
	      Compare comp)
{
	assert(item >= first && item <= last && "heap_tricleUp: item not in range [first, last]");
	if(item == first)
		return false;
	bool retVal = false;
	std::size_t distance = std::distance(first, item);
	std::size_t pDistance = (distance-1)/2;
	while(item != first) {
		RandomIterator parent = first;
		LOG_DEBUG("testing item ", distance, " with its parent ", pDistance, ": comp(*parent, *item) = ", comp(*parent, *item));
		std::advance(parent, pDistance);
		if(comp(*parent, *item)){	//parent < item
			std::swap(*parent, *item);
			retVal = true;
			distance = pDistance;
			item = parent;
		} else {
			break;
		}
		pDistance = (pDistance-1)/2;
	}
	return retVal;
}
/**
 * Performs the tricle down algorithm.
 */
template<typename RandomIterator, typename Compare>
    inline void
    heap_tricleDown(RandomIterator first, RandomIterator last, RandomIterator item,
	      Compare comp)
{
	assert(item >= first && item <= last && "heap_tricleDown: item not in range [first, last]");
	if(item == last)
		return;
	std::size_t distance = std::distance(first, item);
	std::size_t length = std::distance(first, last);
	while(true) {
		std::size_t leftD = distance*2+1;
		if(leftD >= length)
			return;
		RandomIterator left = first;
		std::advance(left, leftD);
		if(comp(*item, *left)){		//item < left
			std::swap(*left, *item);
			item = left;
			distance = leftD;
			continue;
		} else {
			std::size_t rightD = distance*2+2;
			if(leftD >= length)
				return;
			RandomIterator right = first;
			std::advance(right, rightD);
			if(comp(*item, *right)) {		//item < left
				std::swap(*right, *item);
				item = right;
				distance = rightD;
				continue;
			}
		}
		break;
	}
}

/*
 * Updates the item in a sequence so that it regains its heap property.
 * @param first A random access iterator that denotes the beginning of the sequence.
 * @param last A random access iterator that denotes the end of the sequence.
 * @param item A random access iterator that denotes the item that must be updated.
 * @param comp A comparator object/function.
 * @see std::make_heap
 */
template<typename RandomIterator, typename Compare>
    inline void
    fix_heap(RandomIterator first, RandomIterator last, RandomIterator item,
	      Compare comp)
{
	if(!heap_tricleUp(first, last, item, comp))
		heap_tricleDown(first, last, item, comp);
}

} /* namespace n_tools */


#endif /* SRC_TOOLS_HEAP_H_ */
