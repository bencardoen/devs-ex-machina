/**
 * Memory pools
 * @author Ben Cardoen
 */
#ifndef SRC_POOLS_CFPOOL_H_
#define SRC_POOLS_CFPOOL_H_

#include "pools/pools.h"
#include "tools/globallog.h"
#include <iostream>
#include <bitset>

namespace n_pools{

/** 
 * A bitmap of sizeof<t_word>*8 bits, if a bit at MSB + i is set, then ptr
 *  at base_ptr + i is free.
 */
typedef uint64_t t_word;        // << choice is limited here to 32/64.
typedef std::deque<t_word> t_mmap;

constexpr size_t range_word = sizeof(t_word)*8;
// In future, allow dyn bset ? Speed advantage of builtins and shifting is to tempting atm.
static_assert(range_word == 64, "Static size bitset not allocated for this wordsize");


template<typename T>
struct stateful_lane{
        pool_lane<T>    m_lane;
        // Bitmap for the lane. 1 == free.
        t_mmap          m_map;
        
        // Last position where a pool requested a free pointer from.
        size_t          m_word_index;
        stateful_lane(T* raw, size_t sz):m_lane(raw,sz),m_word_index(0)
        {
                std::lldiv_t qr = std::lldiv(sz, range_word);
                for(size_t i = 0; i < (size_t) qr.quot; ++i)
                        m_map.push_back(std::numeric_limits<t_word>::max());
                if(qr.rem)
                        m_map.push_back(std::numeric_limits<t_word>::max()<< (range_word-qr.rem));
        }
        
        constexpr T* begin() const {return m_lane.begin();}
        constexpr T* last() const {return m_lane.last();}
        constexpr T* raw() const {return m_lane.raw();}
        constexpr size_t size() const{return m_lane.size();}
};

/**
 * A dynamic sized pool with a bitmap as freelist.
 * Use case : Random allocation/deallocation patterns.
 * Memory cost = (size / 64). (as in ptrcount / 64).
 * Runtime cost = O( size/64 ) depending on allocation strategy and yet to implement caching of nextptr.
 */
template<typename T>
class CFPool: public PoolInterface<T>
{
        private:
                // Each lane holds [begin,end] T*, and a bitmap of allocated ptrs.
                std::deque<stateful_lane<T>> m_lanes;
                std::size_t m_osize;
                std::size_t m_allocated;
                std::size_t m_lane_index;
                
        public:
                // @pre initsize > 0
                CFPool(size_t initsize):m_osize(initsize),m_allocated(0),m_lane_index(0)
                {
                        T* fblock = (T*) std::malloc(sizeof(T)*(m_osize+1));
                        if(!fblock)
                                throw std::bad_alloc();
                        m_lanes.push_back(stateful_lane<T>(fblock, m_osize));
                        
                }               
                ~CFPool()
                {
                        for(auto& lane : m_lanes)
                                std::free (lane.m_lane.raw());
                }
                
                T* allocate()override
                {
                        // Emulate first-fit alloc.
                        // Remember last position (of lane last used)
                        //      start from that position, wrap around until nothing is found (old pos is reached)
                        // For each lane, remember what word last was used.
                        // If nothing is found in that lane, reset it to zero, else update the value.
                        const size_t old_index = m_lane_index;
                        for(;;){
                                auto& lane = m_lanes[m_lane_index];
                                // We remember which word was last used in this lane, but don't wrap around.
                                // If no free ptr is found, the word_index is reset.
                                size_t j = lane.m_word_index;
                                for(;j<lane.m_map.size();){
                                        t_word& word = lane.m_map[j];
                                        // If word is nonzero, at least 1 ptr can be found.
                                        if(word){
                                                size_t i = n_tools::firstbitset<8>(word);        // leading zeros indicates which ptr is free
                                                T* base = lane.begin();                   
                                                word &= ~(1ull << (range_word-1ull-i));
                                                ++m_allocated;
                                                lane.m_word_index=j;
                                                LOG_DEBUG("CFPOOL:: allocated ::  ", (base + (j*range_word) + i));
                                                return (base + (j*range_word) + i);
                                        }
                                        ++j;
                                }
                                lane.m_word_index=0;
                                ++m_lane_index;
                                m_lane_index %= m_lanes.size();
                                if(m_lane_index == old_index)   // wrap-around.
                                        break;
                        }
                        LOG_DEBUG("CFPool :: out of memory");
                        throw std::bad_alloc(); // Replace with expanding AFTER tests are complete.
                }
                
                void deallocate(T* t)override{
                        // Find pointer can be done faster by using base addr if exp size = fixed.
                        // In general allocated blocks will increment, but this is never guaranteed so
                        // even binary search is hazardous.
                        for(size_t index = 0; index<m_lanes.size();++index){
                                auto& lane = m_lanes[index];
                                long long int dist = t-lane.begin();
                                if( dist >= 0 && t<= lane.last()) // or dist>=0 && dist < lane.size(), eqv in speed as long as short circuit is kept hot.
                                {
                                        std::lldiv_t rowcol = std::lldiv(dist , range_word);
                                        t_word& word = lane.m_map[rowcol.quot];
#ifdef SAFETY_CHECKS
                                        t_word prev = word;
#endif
                                        --m_allocated;
                                        word |= (1ull << (range_word - rowcol.rem - 1ull));
#ifdef SAFETY_CHECKS
                                        if(word == prev){
                                                LOG_ERROR("Dist == ", dist, " quot == ", rowcol.quot ," rem = ",rowcol.rem);
                                                LOG_FLUSH;
                                                throw std::logic_error("Double dealloc");
                                        }
#endif                                  
                                        // Remember hot lane for next alloc ?
                                        //m_lane_index = index;
                                        return;
                                }
                        }
                        LOG_DEBUG("CFPOOL : could not dealloc :", t);
                        throw std::bad_alloc();
                        
                }
                
                constexpr size_t size()const{return m_osize;}
                
                constexpr size_t allocated()const{return m_allocated;}
};


}// EON.


#endif /* SRC_POOLS_CFPOOL_H_ */
