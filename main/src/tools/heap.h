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
 * Performs the trickle up algorithm.
 * Returns true if at least one swap was made.
 */
template<typename RandomIterator, typename Compare>
    bool
    heap_trickleUp(RandomIterator first, RandomIterator last, RandomIterator item,
	      Compare comp)
{
	assert(item >= first && item <= last && "heap_trickleUp: item not in range [first, last]");
	RandomIterator iter = item;
	std::size_t pDistance = std::distance(first, item);
	while(iter > first) {
		pDistance = (pDistance-1)/2;
		RandomIterator parent = first + pDistance;
		LOG_DEBUG("testing item with its parent ", pDistance, ": comp(*parent, *item) = ", comp(*parent, *item));
		if(comp(*parent, *iter)){	//parent < item
			LOG_DEBUG("swapping items ", parent-first, " and ", iter-first);
			std::swap(*parent, *iter);
			iter = parent;
			continue;
		}
		break;
	}
	return (iter != item);
}
/**
 * Performs the trickle down algorithm.
 */
template<typename RandomIterator, typename Compare>
    void
    heap_trickleDown(RandomIterator first, RandomIterator last, RandomIterator item,
	      Compare comp)
{
	assert(item >= first && item <= last && "heap_trickleDown: item not in range [first, last]");
	std::size_t distance = std::distance(first, item);
	std::size_t length = std::distance(first, last);
	while(item < last) {
		LOG_DEBUG("starting test with distance ", distance);
		std::size_t leftD = distance*2+1;
		if(leftD >= length)
			return;
		LOG_DEBUG("leftD: ", leftD, "  rightD: ", leftD+1, "  length: ", length, "  testRight: ", (leftD+1 < length), "  distance: ", distance);
		RandomIterator left = first+leftD;
		if(leftD+1 == length){	//leftD > length is impossible here
			if(comp(*item, *left)){		//item < left && no right item.
				std::swap(*left, *item);
			}
			return;	//left child can't have any children
		}
		RandomIterator right = left+1;
		//swap with the largest value!
		bool useRight = comp(*left, *right);
		RandomIterator test = useRight? right: left;
		std::size_t newD = useRight? leftD+1: leftD;
		if(comp(*item, *test)){		//item < max(left, right)
			std::swap(*test, *item);
			item = test;
			distance = newD;
			continue;
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
	if(!heap_trickleUp(first, last, item, comp))
		heap_trickleDown(first, last, item, comp);
}


template<typename RandomIterator, typename Compare, typename Update>
    bool
    heap_trickleUp(RandomIterator first, RandomIterator last, RandomIterator item,
	      Compare comp, Update upd)
{
	assert(item >= first && item <= last && "heap_trickleUp: item not in range [first, last]");
	RandomIterator iter = item;
	std::size_t pDistance = std::distance(first, item);
	while(iter > first) {
		std::size_t distance = pDistance;
		pDistance = (pDistance-1)/2;
		RandomIterator parent = first + pDistance;
		LOG_DEBUG("testing item with its parent ", pDistance, ": comp(*parent, *item) = ", comp(*parent, *item));
		if(comp(*parent, *iter)){	//parent < item
			LOG_DEBUG("swapping items ", parent-first, " and ", iter-first);
			std::swap(*parent, *iter);
			upd(*parent, pDistance);
			upd(*iter, distance);
			iter = parent;
			continue;
		}
		break;
	}
	return (iter != item);
}

/**
 * Performs the trickle down algorithm.
 */
template<typename RandomIterator, typename Compare, typename Update>
    void
    heap_trickleDown(RandomIterator first, RandomIterator last, RandomIterator item,
	      Compare comp, Update upd)
{
	assert(item >= first && item <= last && "heap_trickleDown: item not in range [first, last]");
	std::size_t distance = std::distance(first, item);
	std::size_t length = std::distance(first, last);
	while(item < last) {
		std::size_t leftD = distance*2+1;
		if(leftD >= length)
			return;
		LOG_DEBUG("leftD: ", leftD, "  rightD: ", leftD+1, "  length: ", length, "  testRight: ", (leftD+1 < length), "  distance: ", distance);
		RandomIterator left = first+leftD;
		if(leftD+1 == length){	//leftD > length is impossible here
			if(comp(*item, *left)){		//item < left && no right item.
				LOG_DEBUG("Swapped left and item");
				std::swap(*left, *item);
				upd(*left, leftD);
				upd(*item, distance);
			}
			return;	//left child can't have any children
		}
		RandomIterator right = left+1;
		//swap with the largest value!
		bool useRight = comp(*left, *right);
		RandomIterator test = useRight? right: left;
		std::size_t newD = useRight? leftD+1: leftD;
		if(comp(*item, *test)){		//item < max(left, right)
			std::swap(*test, *item);
			LOG_DEBUG("Swapped test and item");
			upd(*test, newD);
			upd(*item, distance);
			item = test;
			distance = newD;
			continue;
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
 * @param upd A functor object/function with signature void (ItemType& item, std::size_t newPos);
 * 			where newPos is the new index of item in the sequence.
 * @note It is assumed that the update functor will not change the items in such a way that the comparator may return different results.
 * @note The update functor is only called if a swap is performed.
 * @see std::make_heap
 */
template<typename RandomIterator, typename Compare, typename Update>
    inline void
    fix_heap(RandomIterator first, RandomIterator last, RandomIterator item,
	      Compare comp, Update upd)
{
	if(!heap_trickleUp(first, last, item, comp, upd))
		heap_trickleDown(first, last, item, comp, upd);
}

} /* namespace n_tools */


#endif /* SRC_TOOLS_HEAP_H_ */
