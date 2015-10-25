/**
 * Memory pools
 */
#ifndef SRC_TOOLS_POOLS_H_
#define SRC_TOOLS_POOLS_H_

#include <new>
#include <vector>



namespace n_tools {

/**
 * Interface for Pools. Specialize per pool type.
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
                explicit Pool(size_t psize, size_t nsize=0){;}
                
                /**
                 * Destroy the pool. Deallocates, assumes that either destructor is called here by the 
                 * pool itself or this has happened by the deallocate function.
                 */
                ~Pool(){;}
                
                /**
                 * @return A pointer to the next free (uninitialized) object.
                 * @throw bad_alloc if the pool cannot service the request.
                 */
                Object* allocate(){return nullptr;}
                
                /**
                 * Destroy parameter
                 * @pre O->~Object() has not been called, O was allocated by this pool.
                 * @post O->~Object() is invoked. Memory @O is returned to the pool (but may or may not be returned to the OS).
                 * @throw bad_alloc if O does not belong to this pool.
                 */
                void deallocate(Object* O){
                        ;
                }
};


/**
 * Single use case pool : create (at most) N objects, and keep reusing the memory.
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

/*
 * Partial Specialization for SlabPool.
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
 * Forwarding specialization to malloc/free.
 * Useful for comparing pools wrt n/d perf.
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


} /* namespace n_tools */

#endif /* SRC_TOOLS_POOLS_H_ */
