/**
 * Memory pools
 */
#ifndef SRC_TOOLS_POOLS_H_
#define SRC_TOOLS_POOLS_H_

#include <new>
#include "boost/pool/object_pool.hpp"
#include "boost/pool/pool.hpp"
#include "boost/pool/singleton_pool.hpp"



namespace n_tools {

/**
 * Interface for Pools. 
 * @param Object : Pool provides memory in chunks of sizeof(Object).
 * @param P : the underlying Pool type
 */
template<typename Object, typename P>
class Pool{
        public:
                /**
                 * Create new pool.
                 * @param psize Pool can hold at least psize objects
                 * @param nsize if applicable, set the step size with which the pool increases if it runs out of memory.
                 * @note : nsize can be ignored by the pool, psize is a guarantee that the pool will provide either space for none(alloc fail) or at least psize objects.
                 */
                explicit Pool(size_t psize, size_t nsize=0);
                
                /**
                 * Destroy the pool. Deallocates, assumes that either destructor is called here by the 
                 * pool itself or this has happened by the deallocate function.
                 */
                ~Pool();
                
                /**
                 * @return A pointer to the next free (uninitialized) object.
                 * @throw bad_alloc if the pool cannot service the request.
                 */
                Object* allocate();
                
                /**
                 * Destroy parameter
                 * @pre O->~Object() has not been called, O was allocated by this pool.
                 * @post O->~Object() is invoked. Memory @O is returned to the pool (but may or may not be returned to the OS).
                 * @throw bad_alloc if O does not belong to this pool.
                 */
                void deallocate(Object* O);
};


/**
 * Static pool with very little overhead.
 * Allocates at construction all the memory it will need as indicated by the user,
 * reuses that memory if all ptrs have been collected.
 * @attention: obviously this pool should only be used in a usage pattern where you cycle
 * between alloc(n), dealloc(n) with a predetermined limit on n. The simplicity of this pool
 * allows a severe reduction in runtime.
 */
template<typename T>
class SlabPool{
        private:
                /**
                 * #malloc() - #free()
                 */
                size_t          m_allocated;
                /**
                 * Requested pool size
                 */
                size_t          m_osize;
                /**
                 * Begin address of pool.
                 */
                T* const        m_pool;
                /**
                 * Points to the next free object.
                 */
                T*      m_currptr;
        public:
                explicit SlabPool(size_t poolsize):m_allocated(0),m_osize(poolsize),m_pool((T*) malloc(sizeof(T)*poolsize)),m_currptr(m_pool){
                        if(!m_pool)
                                throw std::bad_alloc();
                }
                
                ~SlabPool()
                {
                        if(m_pool){
                                free(m_pool);
                        }
                }
                
                size_t size()const
                {
                        return m_osize;
                }
                
                size_t allocated()const
                {
                        return m_allocated;
                }
                
                /**
                 * @pre allocated() < size()
                 * @return A pointer to the next free object.
                 * @throw bad_alloc if allocated()>size()
                 */
                T* allocate()
                {
                        if(m_allocated < m_osize){
                                ++m_allocated;
                                return m_currptr++;
                        }
                        else
                        {
                                // In future, allow expanding of pool (but requires a smart (jumping) ptr.
                                throw std::bad_alloc();
                        }
                }
                
                /**
                 * Indicate T* is no longer required.
                 * If the alloc count reaches zero, the pool can be reused.
                 */
                void deallocate(T*)
                {
                        // In future, look up part of pool based on T* range, and mark for reuse iff all ptrs returned.
                        if(m_allocated){
                                --m_allocated;
                                if(!m_allocated)
                                        m_currptr=m_pool;
                        }
                        else{
                                throw std::bad_alloc();
                        }
                }
};

/**
 * Static pool with very little overhead, but applies only to selected use case
 * @see SlabPool.
 */
template<typename Object>
class Pool<Object, SlabPool<Object>>
{
        private:
                SlabPool<Object>        m_pool;
        public:
                /**
                 * Create new pool.
                 * @param psize Pool can hold exactly psize objects
                 * @param nsize = ignored, this is a static pool.
                 */
                explicit Pool(size_t psize, size_t /*nsize*/=0):m_pool(psize){;}
                
                /**
                 * Destroy the pool. Deallocates, assumes that either destructor is called here by the 
                 * pool itself or this has happened by the deallocate function.
                 * @note : RAII, no explicit code required.
                 */
                ~Pool(){;}
                
                /**
                 * @return A pointer to the next free (uninitialized) object.
                 * @throw bad_alloc if the pool cannot service the request.
                 */
                Object* allocate()
                {
                        return m_pool.allocate();
                }
                
                /**
                 * Destroy parameter
                 * @pre O->~Object() has not been called, O was allocated by this pool.
                 * @post O->~Object() is invoked. Memory @O is returned to the pool (but may or may not be returned to the OS).
                 * @throw bad_alloc if O does not belong to this pool.
                 */
                void deallocate(Object* O)
                {
                        m_pool.deallocate(O);
                }
};

/**
 * Forwarding specialisation to malloc/free.
 * Implemented as a pass-through for, among others, comparing to pools.
 */
template<typename Object>
class Pool<Object, std::false_type>
{
          public:
                /**
                 * Create new pool.
                 * @param psize Pool can hold at least psize objects
                 * @param nsize if applicable, set the step size with which the pool increases if it runs out of memory.
                 * @note : nsize can be ignored by the pool, psize is a guarantee that the pool will provide either space for none(alloc fail) or at least psize objects.
                 */
                explicit Pool(size_t /*psize*/, size_t /*nsize*/ = 0){;}
                
                /**
                 * Destroy the pool. Deallocates, assumes that either destructor is called here by the 
                 * pool itself or this has happened by the deallocate function.
                 */
                ~Pool(){;}
                
                /**
                 * @return A pointer to the next free (uninitialized) object.
                 * @throw bad_alloc if the pool cannot service the request.
                 */
                Object* allocate(){
                        return (Object*) std::malloc(sizeof(Object));
                }
                
                /**
                 * Destroy parameter
                 * @pre O->~Object() has not been called, O was allocated by this pool.
                 * @post O->~Object() is invoked. Memory @O is returned to the pool (but may or may not be returned to the OS).
                 * @throw bad_alloc if O does not belong to this pool.
                 */
                void deallocate(Object* O){
                        O->~Object();
                        std::free(O);
                }
};

/**
 * Specialisation for boost object pool.
 * The usage pattern for this pool is create (pool) once, at end of scope let
 * the pool handle destruction. Deallocation is supported but expensive.
 */
template<typename Object>
class Pool<Object, boost::object_pool<Object>>{
        private:
                boost::object_pool<Object> m_pool;
        public:
                /**
                 * Create new pool.
                 * @param psize Pool can hold at least psize objects
                 * @param nsize if applicable, set the step size with which the pool increases if it runs out of memory.
                 * @note : nsize can be ignored by the pool, psize is a guarantee that the pool will provide either space for none(alloc fail) or at least psize objects.
                 */
                explicit Pool(size_t psize, size_t nsize=0):m_pool((nsize == 0)? psize : nsize){}// opool(nsize), so need to convert here.
                
                /**
                 * Destroy the pool. Deallocates, assumes that either destructor is called here by the 
                 * pool itself or this has happened by the deallocate function.
                 */
                ~Pool(){}
                
                /**
                 * @return A pointer to the next free (uninitialized) object.
                 * @throw bad_alloc if the pool cannot service the request.
                 */
                Object* allocate()
                {
                        return m_pool.malloc();
                }
                
                /**
                 * Destroy parameter
                 * @pre O->~Object() has not been called, O was allocated by this pool.
                 * @post O->~Object() is invoked. Memory @O is returned to the pool (but may or may not be returned to the OS).
                 * @throw bad_alloc if O does not belong to this pool.
                 * @attention : this is very expensive for this type of pool. Its intended usage is create pool, allocate objects 
                 * and then self destruct (whole pool) @exit scope.
                 */
                void deallocate(Object* O)
                {
                        O->~Object();
                        m_pool.free(O);
                }
};

/**
 * Specialisation for the generic boost pool.
 * Has slight overhead over new/delete, but can be faster.
 */
template<typename Object> 
class Pool<Object, boost::pool<>>{
        private:
                boost::pool<>   m_pool;
        public:
                /**
                 * Create new pool.
                 * @param psize Pool can hold at least psize objects
                 * @nsize = ignored, the pool grows module psize
                 */
                explicit Pool(size_t psize, size_t /*nsize*/=0):m_pool(sizeof(Object), psize){}
                
                /**
                 * Destroy the pool. Deallocates, assumes that either destructor is called here by the 
                 * pool itself or this has happened by the deallocate function.
                 */
                ~Pool(){}
                
                /**
                 * @return A pointer to the next free (uninitialized) object.
                 * @throw bad_alloc if the pool cannot service the request.
                 */
                Object* allocate()
                {
                        return (Object*) m_pool.malloc();
                }
                
                /**
                 * Destroy parameter
                 * @pre O->~Object() has not been called, O was allocated by this pool.
                 * @post O->~Object() is invoked. Memory @O is returned to the pool (but may or may not be returned to the OS).
                 * @throw bad_alloc if O does not belong to this pool.
                 */
                void deallocate(Object* O)
                {
                        O->~Object();
                        m_pool.free(O);
                }
};



struct DXPoolTag{}; // Needed for singleton pools to differentiate between allocators.
template<typename Object>
using spool =  boost::singleton_pool<DXPoolTag, sizeof(Object)>;
/**
 * Singleton pool, use this if the pool has to be shared between threads.
 */
template<typename Object> 
class Pool<Object, spool<Object>>{
        
        public:
                /**
                 * Create new pool.
                 * All parameters are ignored (template parameters for pooltype).
                 */
                explicit Pool(size_t /*psize*/, size_t /*nsize*/=0){}
                
                /**
                 * Destroy the pool. Deallocates, assumes that either destructor is called here by the 
                 * pool itself or this has happened by the deallocate function.
                 */
                ~Pool(){}
                
                /**
                 * @return A pointer to the next free (uninitialized) object.
                 * @throw bad_alloc if the pool cannot service the request.
                 */
                Object* allocate()
                {
                        return (Object*) spool<Object>::malloc();
                }
                
                /**
                 * Destroy parameter
                 * @pre O->~Object() has not been called, O was allocated by this pool.
                 * @post O->~Object() is invoked. Memory @O is returned to the pool (but may or may not be returned to the OS).
                 * @throw bad_alloc if O does not belong to this pool.
                 */
                void deallocate(Object* O)
                {
                        O->~Object();
                        spool<Object>::free(O);
                }
};

// Instantiation so that compile errors are detected if the code changes (e.g) not using a pool type in the project and introducing a bug
// for that particular pool could go unnoticed a long time.
template class Pool<int, boost::object_pool<int>>;
template class Pool<int, boost::pool<>>;
template class Pool<int, SlabPool<int>>;
template class Pool<int, std::false_type>;
template class Pool<int, spool<int>>;

template<typename Object>
using ObjectPool = Pool<Object, SlabPool<Object>>;

template<typename T>
ObjectPool<T>&
getPool()
{
        thread_local ObjectPool<T> pool(400);
        return pool;
}

} /* namespace n_tools */

#endif /* SRC_TOOLS_POOLS_H_ */
