/*
 * This file is part of the DEVS Ex Machina project.
 * Copyright 2014 - 2015 University of Antwerp
 * https://www.uantwerpen.be/en/
 * Licensed under the EUPL V.1.1
 * A full copy of the license is in COPYING.txt, or can be found at
 * https://joinup.ec.europa.eu/community/eupl/og_page/eupl
 *      Author: Ben Cardoen
 */

#ifndef SRC_POOLS_POOLS_H_
#define SRC_POOLS_POOLS_H_

#include <new>
#include <cstdlib>
#include "boost/pool/object_pool.hpp"
#include "boost/pool/pool.hpp"
#include "boost/pool/singleton_pool.hpp"
#include <thread>
#include <atomic>
#include <mutex>
#include <vector>
#include <deque>
#include <set>
#include "tools/globallog.h"
#include "tools/flags.h"

// TODO: this code is a perfect example where C++17's transactional support would simplify most of this code to only increment/decrement operations,
// and make it perfectly safe for any project to use. Shame it's not stable enough to use (yet).

/**
 * A pool is thread local. 
 * At compile time the (static) types of the objects to pool are registered, 
 * the pool itself is only created at the first call to getPool<T>. 
 * It is destructed after the thread exits and returns all
 * memory to the OS. Note that this does NOT imply that the destructor is called 
 * for objects that weren't deallocated. 
 * For T with trivial destructor, this is a non-issue, else ensure deallocate is called.
 * 
 * @attention : Call the pool with the __declared__ (static) type of the object, NEVER the
 * dynamic type.
 * 
 * Usage:
 *      {
 *      Object* o  = createPooledObject<T>(args);
 *      destroyPooledObject<T>(o);
 *      }
 * 
 * This usually (+- logging, statistics etc) resolves to :
 *      {
 *      PoolInterface<T>* pool = getPool<T>();      
 *      Object* o = new (pool->allocate()) Object(constructor_params);
 *      ...
 *      getPool<T>()->deallocate(o);
 *      }
 *
 */



namespace n_pools {

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
        /**
         * @return A contiguous block of sizeof(T) bytes, aligned for an object of type T.
         * @throws bad_alloc if allocation fails for any reason.
         */
        virtual T* allocate() = 0;
        
        /**
         * @pre *t is an instance of type T, [t, ++t) is allocated by this pool.
         * @post ~T() is called before memory is returned to the pool (may or may not be returned to OS).
         */
        virtual void deallocate(T* t) = 0;
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
                virtual ~Pool();
                
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
 * @param raw possibly unaligned pointer to T.
 * @pre std::alignment_of<T>::value >=8 && power of 2.
 * @post ((uintptr_t)returnvalue  % std::alignment_of<T>::value) == 0
 */

template<typename T>
constexpr
T* align_ptr(T* raw)
{
        constexpr size_t algn = std::alignment_of<T>::value;
        static_assert(algn > 7 && n_tools::is_power_2(algn) , "Invalid alignment.");
        uintptr_t ir = (uintptr_t) raw;
        return ((!(ir%algn)) ? raw : (T*) ((ir+algn)&(~(algn-1ull))));
        /* Constexpr pre c++14 does not allow if/else, but the ternary operator is allowed. */
/**     ^^ == vv            
        if((ir%algn)==0)
                return raw;
        else{
                ir += algn;
                ir &= ~(algn-1ull);
                return (T*) ir;
        }
 */
}

/**
 * Helper structure, defines a contiguous slice of memory that a pool uses.
 */
template<typename T>
struct pool_lane{
private:
        // Pointer returned by malloc. Only this one can be returned to free.
        T* m_raw_base;
        // First valid (aligned ptr)
        T* m_base_addr;
        // Lane can hold exactly m_size objects, so m_base_addr += (m_size-1) points to the last object
        size_t m_size;
public:
        pool_lane():m_raw_base(nullptr),m_base_addr(nullptr),m_size(0){;}
        /**
         * Construct a new lane.
         * @param pt : pointer returned by a malloc() call with allocated size at least sz+1
         * @param sz : nr of objects this lane needs to provide.
         */
        constexpr pool_lane(T* pt, size_t sz):
                m_raw_base(pt),
                m_base_addr(align_ptr(pt)),
                m_size(sz){;}
        
        constexpr size_t size()const{return m_size;}
        constexpr T* begin()const{return m_base_addr;}
        
        /**
         * In STL end() points to the last object + 1, so be explicit about what we return here.
         */
        constexpr T* last()const{return m_base_addr + (m_size-1);}
        
        /**
         * Allocated pointer, only this can be returned to free.
         */
        constexpr T* raw()const{return m_raw_base;}
};

///// ARENA POOLS
/**
 * Static (size) pool with very little overhead.
 * Allocates at construction all the memory it will need as indicated by the user,
 * reuses that memory if all ptrs have been collected.
 * @attention: obviously this pool should only be used in a usage pattern where you cycle
 * between alloc(n), dealloc(n) with a predetermined limit on n. The simplicity of this pool
 * allows a severe reduction in runtime.
 * @note : this pool is useful in testing to detect increasing leakage and/or overdeallocation.
 */
template<typename T>
class SlabPool:public PoolInterface<T>{
        private:
                /**
                 * Return the nr of allocated objects.
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
                explicit SlabPool(size_t poolsize):m_allocated(0),m_osize(poolsize),m_pool((T*) malloc(sizeof(T)*(poolsize+1))),m_currptr(m_pool){
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
                 * O(1)
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
                        throw std::bad_alloc();
                }
                
                /**
                 * O(1)
                 * Indicate T* is no longer required.
                 * If the alloc count reaches zero, the pool can be reused.
                 */
                void deallocate(T* t)override
                {
                        t->~T();
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
                // Tracks parity of the pool alloc/dealloc.
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
                explicit DynamicSlabPool(size_t poolsize):
                                m_allocated(0),m_osize(poolsize),
                                m_slabsize(poolsize),m_current_lane(0)
                {
                        T * fblock = (T*) malloc(sizeof(T)*(poolsize+1));
                        if(! fblock)
                                throw std::bad_alloc();
                        m_pools.push_back(pool_lane<T>(fblock, poolsize));
                        m_currptr = fblock;
                }
                
                ~DynamicSlabPool()
                {
                        for(auto& lane : m_pools)
                                std::free(lane.raw());
                }
                
                // Current nr of objects this pool can (in an empty state) allocate.
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
                 * @throw bad_alloc if expanding memory fails.
                 */
                T* allocate()override
                {
                        ++m_allocated;
                        T* next = m_currptr;
                        if(m_currptr == m_pools[m_current_lane].last()){        // eolane
                                if(m_current_lane == m_pools.size()-1){         // last lane == curr
                                        T* nextlane = (T*) std::malloc( sizeof(T) * (1+m_osize));
                                        if(!nextlane)
                                                throw std::bad_alloc();
                                        m_pools.push_back(pool_lane<T>(nextlane, m_osize));
                                        m_osize*=2;
                                }
                                m_currptr = m_pools[++m_current_lane].begin();  // skip to next lane
                        }
                        else{
                                ++m_currptr;
                        }
                        return next;
                }
                
                /**
                 * Indicate T* is no longer required.
                 * If the alloc count reaches zero, the pool can be reused.
                 */
                void deallocate(T* t)override
                {
                        // In future, look up part of pool based on T* range, and mark for reuse iff all ptrs returned.
                        // Take care here to jump backwards across lanes as well.
                        if(m_allocated){
                                t->~T();
                                --m_allocated;
                                if(!m_allocated){  // allocated == 0, iow pool is free, reset counters.
                                        m_current_lane = 0;
                                        m_currptr=m_pools[0].begin();
                                }
                        }
                        else{
                                // Can't deallocate what we don't have allocated
                                throw std::bad_alloc();
                        }
                }
};


//// General pools

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
                
                /**
                 * Blocks of slabsize objects.
                 */
                std::vector<pool_lane<T>> m_pools;
                
                /**
                 * LIFO freelist.
                 */
                std::deque<T*>  m_free;
                
        public:
                explicit StackPool(size_t poolsize):m_osize(poolsize)
                {
                        T * fblock = (T*) malloc(sizeof(T)*(poolsize+1));
                        if(! fblock)
                                throw std::bad_alloc();
                        m_pools.push_back(pool_lane<T>(fblock, poolsize));
                        for(size_t i = 0; i<poolsize; ++i)
                                m_free.push_back(fblock++);
                }
                
                ~StackPool()
                {
                        for(auto lane : m_pools)
                                std::free(lane.raw());
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
                                T * fblock = (T*) malloc(sizeof(T)*(m_osize+1));
                                if(!fblock)
                                        throw std::bad_alloc();
                                m_pools.push_back(pool_lane<T>(fblock, m_osize));
                                for(size_t i = 0; i < m_osize-1; ++i)
                                        m_free.push_back(fblock++);
                                m_osize *= 2;
                                assert(fblock == m_pools.back().last());
                                return fblock;
                        }
                }
                
                /**
                 * Indicate T* is no longer required.
                 */
                void deallocate(T* t)override
                {
                        if(m_free.size()!=m_osize){
                                t->~T();
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
#ifdef SAFETY_CHECKS
        std::set<Object*> m_objList;
#endif
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
//                ~Pool(){;}
                
                /**
                 * @return A pointer to the next free (uninitialized) object.
                 * @throw bad_alloc if the pool cannot service the request.
                 */
                Object* allocate()override
                {
                        Object* obj = (Object*) std::malloc(sizeof(Object));
#ifdef SAFETY_CHECKS
                        // If this check fails, malloc &| heap are severely corrupted.
                        if(! (m_objList.insert(obj).second)){
                                LOG_ERROR("Pointer returned from allocate() still not accounted for :: severe memory corruption.", obj);
                                throw std::bad_alloc();
                        }       
#endif
                        return obj;
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
#ifdef SAFETY_CHECKS
                        // Can be useful to detect cross-thread dealloc. (double free would be caught anyway).
                        if(m_objList.erase(O)==0){
                                LOG_ERROR("Double free :: ", O, " was either never allocated by this pool, or deallocated twice.");
                                throw std::bad_alloc();
                        }
#endif
                        std::free(O);
                }

                ~Pool()
                {
#ifdef SAFETY_CHECKS
                        auto id = std::this_thread::get_id();
                        for(Object* obj:m_objList)
                                LOG_ERROR("thread ", id, " Didn't deallocate object ", obj);
                        assert(m_objList.empty() && "You have a memory leak somewhere... Please see the log file.");
#endif
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
 * Too slow to use in our setting.
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


/**
 * Pool configuration code. 
 * The problem solved here is 2-part: 
 *  - Set an appropriate pool type depending on what simulation type a kernel is running.
 *  - Allow each kernel its own dedicated pool, per object class. 
 * Since performance is key, we want this resolved at compile time.
 */

// Allow compile time tuning of the pool types for benchmarking.
// Single core. This usually means heavy cycles of allocating a lot, then deallocating it all with little
// persisting beyond a single step. An Arena type pool is perfect for this, once the initial peak has been reached it's just pointer arithmetic.
template<typename Object>
#ifdef POOL_SINGLE_ARENA
using SCObjectPool = SlabPool<Object>;
#elif POOL_SINGLE_ARENA_DYNAMIC
using SCObjectPool = DynamicSlabPool<Object>;
#elif POOL_SINGLE_STL
// can use STL as well, but it's too slow.
using SCObjectPool = Pool<Object,std::false_type>;
#else
// Boost, same as multicore. Don't rely on the cyclic pattern, but at a cost.
using SCObjectPool = Pool<Object, boost::pool<>>;
#endif


// Multi core. With conservative, at least some of the pattern will be cyclic, but not enough. For optimistic there is no
// predicting what happens, so a general purpose pool is needed. 
// This (excessive memory usage), and not reverts or synchronization, is really what holds optimistic back.
template<typename Object>
#ifdef POOL_MULTI_STL
using MCObjectPool = Pool<Object,std::false_type>;
#else
using MCObjectPool = Pool<Object, boost::pool<>>;
#endif


/**
 * Registers a pool per thread, forward decl.
 * @pre main has called getMainThreadID() at least once as first caller.
 */
template<typename T>
PoolInterface<T>* 
initializePool(size_t psize);

/**
 * Get the thread local pool for type T.
 * @return an unsynchronized pool that allocates memory chunks sizeof(T).
 */
template<typename T>
PoolInterface<T>*
getPool()
{
        thread_local constexpr size_t initpoolsize = 5000;     // Could be just static.
        thread_local std::unique_ptr<PoolInterface<T>> pool(initializePool<T>(initpoolsize));
        return pool.get();
}


/// Helper functions

/**
 * Record the id of the first thread entering this function, and return that value for all calls.
 * @pre main() enters this function first
 * @deprecated : use isMain()/setMain()
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
 * Use isMain() to check if main is set, or setMain() to do so.
 */
inline
bool& isMainImpl__()
{
        thread_local bool is_main = false;
        return is_main;
}

/**
 * @pre setMain() has been called by, you guessed it, main().
 */
inline
bool isMain()
{
        return isMainImpl__();
}

inline
void setMain(){isMainImpl__()=true;}

/**
 * Registers a pool per thread.
 * @pre main has called getMainThreadID() at least once as first caller.
 * @param psize : initial size of the pool.
 */
template<typename T>
PoolInterface<T>* 
initializePool(size_t psize)
{
        return ( isMain() ? (PoolInterface<T>*) new SCObjectPool<T>(psize) : (PoolInterface<T>*) new MCObjectPool<T>(psize) );
}




} /* namespace n_pools */

#endif /* SRC_POOLS_POOLS_H_ */
