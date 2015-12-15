/*
 * This file is part of the DEVS Ex Machina project.
 * Copyright 2014 - 2015 University of Antwerp
 * https://www.uantwerpen.be/en/
 * Licensed under the EUPL V.1.1
 * A full copy of the license is in COPYING.txt, or can be found at
 * https://joinup.ec.europa.eu/community/eupl/og_page/eupl
 *      Author: Ben Cardoen
 */

#ifndef SRC_TOOLS_SHAREDVECTOR_H_
#define SRC_TOOLS_SHAREDVECTOR_H_

#include <vector>
#include <mutex>
#include <atomic>
#include <array>
#include <deque>

namespace n_tools {

/**
 * Shared Vector of type T, constant sized. (no constexpr, just const).
 * Each entry is protected by a dedicated lock, concurrent access to differing
 * indices will not block.
 */
template<typename T>
class SharedVector
{
private:
	std::vector<T> 	m_vector;

	// Deque so we can use mutex safely. (will not copy)
	std::deque<std::mutex> m_locks;
public:
	SharedVector()=delete;
	SharedVector(const SharedVector&) =delete;
	SharedVector(size_t size, T initvalue):m_vector(size, initvalue),m_locks(size){
	}

	/**
	 * Lock entry @index.
	 * @pre index < size();
	 */
	void lockEntry(size_t index){
		m_locks[index].lock();
	}

	/**
	 * Unlock entry @index
	 * @pre index<size
	 */
	void unlockEntry(size_t index){
		m_locks[index].unlock();
	}

	/**
	 * Get value @index.
	 * @pre index < size().
	 */
	T get(std::size_t index){
		//std::lock_guard<std::mutex> lockentry{m_locks[index]};
		return m_vector[index];
	}

	/**
	 * Set current @ index to value.
	 * @pre index < size().
	 */
	void set(std::size_t index, const T& value){
		//std::lock_guard<std::mutex> lockentry{m_locks[index]};
		m_vector[index]=value;
	}

	/**
	 * @synchronized not, the container is const sized.
	 */
	std::size_t
	size()const {return m_vector.size();}

	/**
	 * Explicitly not virtual, do not subclass.
	 */
	~SharedVector(){;}
};

template<typename T>
class SharedAtomic{
private:
        const size_t  m_size;
        std::vector<std::atomic<T>> m_atomics;
        SharedAtomic()=delete;
        SharedAtomic(const SharedAtomic&)=delete;
        SharedAtomic(const SharedAtomic&&)=delete;
        SharedAtomic& operator=(const SharedAtomic&)=delete;
        SharedAtomic& operator=(const SharedAtomic&&)=delete;
public:
        ~SharedAtomic()=default;
        explicit SharedAtomic(size_t sz, const T& value):m_size(sz),m_atomics(sz){
                for(auto& at : m_atomics)
                        at.store(value);
        }
        T get(size_t index, std::memory_order ordering=std::memory_order_seq_cst){
#ifdef SAFETY_CHECKS
                return m_atomics.at(index).load(ordering);
#else
                return m_atomics[index].load(ordering);
#endif
        }
        
        void set(size_t index, const T& val){
#ifdef SAFETY_CHECKS
                m_atomics.at(index).store(val);
#else
                m_atomics[index].store(val);
#endif
        }
        
        /**
         * Nr of entries.
         * @return 
         */
        constexpr size_t size()const{return m_size;}
};

} /* namespace n_tools */

#endif /* SRC_TOOLS_SHAREDVECTOR_H_ */
