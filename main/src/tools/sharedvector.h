/*
 * sharedvector.h
 *
 *  Created on: 4 May 2015
 *      Author: Ben Cardoen
 */

#ifndef SRC_TOOLS_SHAREDVECTOR_H_
#define SRC_TOOLS_SHAREDVECTOR_H_

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
	// Have to be very careful here, std::mutex is non movable, non copyable,
	// since size is invariant after construction, and we only access by &,
	// we're safe.
	std::vector<std::mutex> m_locks;
public:
	SharedVector()=delete;
	SharedVector(const SharedVector&) =delete;
	SharedVector(size_t size, T initvalue):m_vector(size, initvalue),m_locks(size){
		;
	}

	/**
	 * Get value @index.
	 * @pre index < size().
	 * @synchronized
	 */
	T get(std::size_t index){
		std::lock_guard<std::mutex> lockentry{m_locks[index]};
		return m_vector[index];
	}

	/**
	 * Set current @ index to value.
	 * @pre index < size().
	 * @synchronized.
	 */
	void set(std::size_t index, const T& value){
		std::lock_guard<std::mutex> lockentry{m_locks[index]};
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

} /* namespace n_tools */

#endif /* SRC_TOOLS_SHAREDVECTOR_H_ */
