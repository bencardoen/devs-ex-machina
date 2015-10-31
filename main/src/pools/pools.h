/**
 * Memory pools
 */
#ifndef SRC_POOLS_POOLS_H_
#define SRC_POOLS_POOLS_H_

#include <new>
#include "boost/pool/object_pool.hpp"
#include "boost/pool/pool.hpp"
#include "boost/pool/singleton_pool.hpp"
#include <thread>
#include <atomic>
#include <mutex>
#include <deque>

/**
 * Here be dragons...
 * The pools are NOT intended to be shared between threads (which obliterates any hint of performance)
 */



namespace n_pools {

/**
 * Helper structure, defines a contigous slice of memory that a pool uses.
 */
template<typename T>
struct pool_lane{
private:
        T* m_base_addr;
        // Lane can hold exactly m_size objects, so m_base_addr += (m_size-1) points to the last object
        size_t m_size;
public:
        pool_lane():m_base_addr(nullptr),m_size(0){;}
        constexpr pool_lane(T* pt, size_t sz):m_base_addr(pt),m_size(sz){;}
        constexpr size_t size()const{return m_size;}
        constexpr T* begin()const{return m_base_addr;}
        
        /**
         * In STL end() points to the last object + 1, so be explicit about what we return here.
         */
        constexpr T* last()const{return m_base_addr + (m_size-1);}
};

/**
 * Record the id of the first thread entering this function, and return that value for all calls.
 * @pre main() enter this function first
 */
inline
std::thread::id getMainThreadID()
{
        static std::thread::id main_id;
        static std::once_flag flagid;
        std::call_once(flagid, [&]()->void{main_id=std::this_thread::get_id();});
        return main_id;
}

/**
 * This class erases the type of pool in use, allowing threads to register their own pools.
 */
template<typename T>
class PoolInterface
{
public:
        typedef T t_type_pooled;
        PoolInterface()=default;
        virtual ~PoolInterface(){;}
        virtual T* allocate() = 0;
        virtual void deallocate(T*) = 0;
};

/**
 * Interface for Pools. 
 * @param Object : Pool provides memory in chunks of sizeof(Object).
 * @param P : the underlying Pool type
 */
template<typename Object, typename P>
class Pool:public PoolInterface<Object>{
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
                virtual Object* allocate()override;
                
                /**
                 * Destroy parameter
                 * @pre O->~Object() has not been called, O was allocated by this pool.
                 * @post O->~Object() is invoked. Memory @O is returned to the pool (but may or may not be returned to the OS).
                 * @throw bad_alloc if O does not belong to this pool.
                 */
                virtual void deallocate(Object* O)override;
};


/**
 * Static pool with very little overhead.
 * Allocates at construction all the memory it will need as indicated by the user,
 * reuses that memory if all ptrs have been collected.
 * @attention: obviously this pool should only be used in a usage pattern where you cycle
 * between alloc(n), dealloc(n) with a predetermined limit on n. The simplicity of this pool
 * allows a severe reduction in runtime.
 * @note : this pool by its very definition will detect over-deallocation.
 */
template<typename T>
class SlabPool:public PoolInterface<T>{
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
                T* allocate()override
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
                void deallocate(T*)override
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
 * A version of the SlabPool that expands if its limits are reached. 
 * Use only for cyclic allocate(n)/deallocate(n) patterns.
 */
template<typename T>
class DynamicSlabPool:public PoolInterface<T>{
         private:
                /**
                 * #malloc() - #free()
                 */
                size_t          m_allocated;
                /**
                 * Total size of available + allocated blocks.
                 */
                size_t          m_osize;
                
                /**
                 * Nr of objects per contiguous block of memory (lane)
                 */
                size_t          m_slabsize;
                
                /**
                 * Index of lane in use. Invariant : all ptrs < m_pools[m_current_lane] are allocated.
                 */
                size_t          m_current_lane;
                
                std::vector<pool_lane<T>> m_pools;
                
                /**
                 * Points to the next free object.
                 */
                T*      m_currptr;
        public:
                explicit DynamicSlabPool(size_t poolsize):m_allocated(0),m_osize(poolsize),m_slabsize(poolsize),m_current_lane(0)
                {
                        T * fblock = (T*) malloc(sizeof(T)*poolsize);
                        if(! fblock)
                                throw std::bad_alloc();
                        m_pools.push_back(pool_lane<T>(fblock, poolsize));
                        m_currptr = m_pools[0].begin();
                }
                
                ~DynamicSlabPool()
                {
                        for(auto lane : m_pools)
                                std::free(lane.begin());
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
                 * @return A pointer to the next free object.
                 * If allocated() == size(), tries to grab a new block of memory to allocate.
                 * @throw bad_alloc if expanding memory fails.
                 */
                T* allocate()override
                {
                        if(m_allocated < m_osize){
                                ++m_allocated;
                                T * npointer = m_currptr;
                                if(m_currptr == m_pools[m_current_lane].last()){        // Make sure we skip to the next lane
                                        if(m_current_lane < m_pools.size()-1)           // if there is one
                                                m_currptr = m_pools[++m_current_lane].begin();
                                        else{
                                                m_currptr = nullptr;                    // Can be left out, going to realloc next time.
                                        }
                                }
                                else{
                                        ++m_currptr;
                                }
                                return npointer;
                        }
                        else
                        {       
                                T * fblock = (T*) malloc(sizeof(T)*m_slabsize);
                                if(!fblock)
                                        throw std::bad_alloc();
                                m_pools.push_back(pool_lane<T>(fblock, m_slabsize));
                                ++m_current_lane;
                                m_osize += m_pools[m_current_lane].size();
                                m_currptr = m_pools[m_current_lane].begin();
                                ++m_allocated;
                                return m_currptr++;
                        }
                }
                
                /**
                 * Indicate T* is no longer required.
                 * If the alloc count reaches zero, the pool can be reused.
                 */
                void deallocate(T*)override
                {
                        // In future, look up part of pool based on T* range, and mark for reuse iff all ptrs returned.
                        // Take care here to jump backwards across lanes as well.
                        if(m_allocated){
                                --m_allocated;
                                if(!m_allocated){
                                        m_current_lane = 0;
                                        m_currptr=m_pools[0].begin();
                                }
                        }
                        else{
                                throw std::bad_alloc();
                        }
                }
};


/**
 * Expanding pool with a stack as free-list.
 * In principle can decrease page faults by reusing frequently used pointers, but this depends
 * on usage.
 */
template<typename T>
class StackPool:public PoolInterface<T>{
         private:
                /**
                 * Total size of available + allocated blocks.
                 */
                size_t          m_osize;
                
                size_t          m_slabsize;                
                
                /**
                 * Blocks of slabsize objects.
                 */
                std::vector<pool_lane<T>> m_pools;
                
                /**
                 * LIFO freelist.
                 */
                std::deque<T*>  m_free;
                
        public:
                explicit StackPool(size_t poolsize):m_osize(poolsize),m_slabsize(poolsize)
                {
                        T * fblock = (T*) malloc(sizeof(T)*poolsize);
                        if(! fblock)
                                throw std::bad_alloc();
                        m_pools.push_back(pool_lane<T>(fblock, poolsize));
                        for(size_t i = 0; i<poolsize; ++i)
                                m_free.push_back(fblock++);
                }
                
                ~StackPool()
                {
                        for(auto lane : m_pools)
                                std::free(lane.begin());
                }
                
                constexpr size_t size()const
                {
                        return m_osize;
                }
                
                constexpr size_t allocated()const
                {
                        return size()-m_free.size();
                }
                
                /**
                 * @return A pointer to the next free object.
                 * If allocated() == size(), tries to grab a new block of memory to allocate.
                 * @throw bad_alloc if expanding memory fails.
                 */
                T* allocate()override
                {
                        if(m_free.size() != 0){
                                T* next = m_free.back();
                                m_free.pop_back();
                                return next;
                        }
                        else{       
                                T * fblock = (T*) malloc(sizeof(T)*m_slabsize);
                                if(!fblock)
                                        throw std::bad_alloc();
                                m_pools.push_back(pool_lane<T>(fblock, m_slabsize));
                                m_osize += m_pools.back().size();
                                // Push any except the last (which we immediately need)
                                for(size_t i = 0; i < m_slabsize-1; ++i)
                                        m_free.push_back(fblock++);
                                return fblock;
                        }
                }
                
                /**
                 * Indicate T* is no longer required.
                 */
                void deallocate(T* t)override
                {
                        if(m_free.size()!=m_osize){
                                m_free.push_back(t);
                        }
                        else{   // Usually a sign of either double free or crossover between pools.
                                throw std::bad_alloc();
                        }
                }
};

/**
 * Forwarding specialisation to malloc/free.
 * Implemented as a pass-through for, among others, comparing to pools.
 */
template<typename Object>
class Pool<Object, std::false_type>:public PoolInterface<Object>
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
                Object* allocate()override
                {
                        return (Object*) std::malloc(sizeof(Object));
                }
                
                /**
                 * Destroy parameter
                 * @pre O->~Object() has not been called, O was allocated by this pool.
                 * @post O->~Object() is invoked. Memory @O is returned to the pool (but may or may not be returned to the OS).
                 * @throw bad_alloc if O does not belong to this pool.
                 */
                void deallocate(Object* O)override
                {
                        O->~Object();
                        std::free(O);
                }
};

/**
 * Specialisation for the generic boost pool.
 * Has slight overhead over new/delete, but can be faster.
 */
template<typename Object> 
class Pool<Object, boost::pool<>>:public PoolInterface<Object>{
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
                Object* allocate()override
                {
                        return (Object*) m_pool.malloc();
                }
                
                /**
                 * Destroy parameter
                 * @pre O->~Object() has not been called, O was allocated by this pool.
                 * @post O->~Object() is invoked. Memory @O is returned to the pool (but may or may not be returned to the OS).
                 * @throw bad_alloc if O does not belong to this pool.
                 */
                void deallocate(Object* O)override
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
class Pool<Object, spool<Object>>:public PoolInterface<Object>{
        
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
                Object* allocate()override
                {
                        return (Object*) spool<Object>::malloc();
                }
                
                /**
                 * Destroy parameter
                 * @pre O->~Object() has not been called, O was allocated by this pool.
                 * @post O->~Object() is invoked. Memory @O is returned to the pool (but may or may not be returned to the OS).
                 * @throw bad_alloc if O does not belong to this pool.
                 */
                void deallocate(Object* O)override
                {
                        O->~Object();
                        spool<Object>::free(O);
                }
};

// Instantiation so that compile errors are detected if the code changes (e.g) not using a pool type in the project and introducing a bug
// for that particular pool could go unnoticed a long time.
template class Pool<int, boost::pool<>>;
template class Pool<int, std::false_type>;

template<typename Object>
using ObjectPool = DynamicSlabPool<Object>;

/**
 * Get the thread local pool for type T.
 * @return an unsynchronized pool that allocates memory chunks sizeof(T).
 */
template<typename T>
ObjectPool<T>&
getPool()
{
        thread_local ObjectPool<T> pool(6400); // ~ roughly 10 pages worth of messages, good enough for w80 iconnect.
        return pool;
}

} /* namespace n_pools */

#endif /* SRC_POOLS_POOLS_H_ */
