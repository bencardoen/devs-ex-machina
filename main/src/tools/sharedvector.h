/*
 * sharedvector.h
 *
 *  Created on: 4 May 2015
 *      Author: ben
 */

#ifndef SRC_TOOLS_SHAREDVECTOR_H_
#define SRC_TOOLS_SHAREDVECTOR_H_

namespace n_model {

/**
 * Shared Vector of type T, constant sized. (no constexpr, just const).
 * Single writer - Multiple Reader problem with a twist:
 * For N clients, each will only ~write~ his own value, never another.
 * Each will only ever read other values, never his own.
 * @brief Provides single operation synchronized access to const sized shared container.
 */
template<typename T>
class SharedVector
{
private:
	std::vector<T> 	m_vector;
	std::mutex	m_lock;
public:
	SharedVector()=delete;
	SharedVector(const SharedVector&) =delete;
	SharedVector(size_t size, T initvalue):m_vector(size, initvalue){
		;
	}

	/**
	 * Get ~copy~ of value @index.
	 * @pre index < size().
	 * @synchronized
	 */
	T get(std::size_t index){
		std::lock_guard<std::mutex> lockvec(m_lock);
		return m_vector[index];
	}

	/**
	 * Set current @ index to value.
	 * @pre index < size().
	 * @synchronized.
	 */
	void set(std::size_t index, const T& value){
		std::lock_guard<std::mutex> lockvec(m_lock);
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

} /* namespace n_model */

#endif /* SRC_TOOLS_SHAREDVECTOR_H_ */
