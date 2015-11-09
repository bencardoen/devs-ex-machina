/**
 * Memory pools
 */
#ifndef SRC_POOLS_CFPOOL_H_
#define SRC_POOLS_CFPOOL_H_

#include "pools/pools.h"
#include "tools/globallog.h"
#include <iostream>
#include <bitset>

namespace n_pools{

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
                // Ptype : 1 lane. Need to figure out how to support variable sized lanes (1 big map, or map per lane)
                std::deque<pool_lane<T>> m_lanes;
                std::size_t m_osize;
                std::size_t m_allocated;
                
                /** A bitmap of sizeof<t_word>*8 bits, if a bit at MSB + i is set, then ptr
                 *  at base_ptr + i is free.
                 */
                typedef uint64_t t_word;        // << choice is limited here to 32/64.
                typedef std::deque<t_word> t_mmap;
                
                t_mmap  m_memory_map;
                
                size_t  m_word_range;
                
        public:
                // @pre initsize > 0
                CFPool(size_t initsize):m_osize(initsize),m_allocated(0),m_word_range(sizeof(t_word)*8)
                {
                        T* fblock = (T*) std::malloc(sizeof(T)*m_osize);
                        if(!fblock)
                                throw std::bad_alloc();
                        m_lanes.push_back(pool_lane<T>(fblock, m_osize));
                        std::lldiv_t qr = std::lldiv(m_osize, m_word_range);
                        for(size_t i = 0; i < (size_t) qr.quot; ++i)
                                m_memory_map.push_back(std::numeric_limits<t_word>::max());
                        if(qr.rem)
                                m_memory_map.push_back(std::numeric_limits<t_word>::max()<< (m_word_range-qr.rem));
                }               
                ~CFPool()
                {
                        for(auto& lane : m_lanes)
                                std::free (lane.begin());
                }
                
                T* allocate()override
                {
                        // We can speed this up if dealloc remembers which lane+word was last touched,
                        // and alloc can immediately start from that lane+word
                        // For better results, keep a shortlist of lanes / words instead of only the last.
                        // We can also provide a cutoff, if we don't find a free pointer 'fast' enough,
                        // get a new block (dangerous in pathological cases).
                        size_t j = 0;
                        for(t_word& word : m_memory_map){
                                if(word){
                                        size_t i = n_tools::firstbitset<8>(word);        // leading zeros.
                                        T* base = m_lanes[0].begin();                   // todo, link to lane
                                        word &= ~(1ull << (m_word_range-1ull-i));
                                        ++m_allocated;
                                        return (base + (j*m_word_range) + i);
                                }
                                ++j;
                        }
                        throw std::bad_alloc(); // Replace with expanding AFTER tests are complete.
                }
                
                void deallocate(T* t)override{
                        // Find pointer can be done faster by using base addr if exp size = fixed.
                        for(auto& lane : m_lanes){
                                long long int dist = t-lane.begin();
                                if( dist >= 0 && t<= lane.last()) // or dist>=0 && dist < lane.size(), eqv in speed as long as short circuit is kept hot.
                                {
                                        std::lldiv_t rowcol = std::lldiv(dist , m_word_range);
                                        t_word& word = m_memory_map[rowcol.quot];
#ifdef SAFETY_CHECKS
                                        
                                        t_word prev = word;
                                        --m_allocated;
                                        
#endif
                                        word |= (1ull << (m_word_range - rowcol.rem - 1ull));
#ifdef SAFETY_CHECKS
                                        if(word == prev){
                                                LOG_ERROR("Dist == ", dist, " quot == ", rowcol.quot ," rem = ",rowcol.rem);
                                                LOG_FLUSH;
                                                throw std::logic_error("Double dealloc");
                                        }
#endif
                                        // TODO remember this position for next allocate.
                                        return;
                                }
                        }
                        throw std::bad_alloc();
                        // not found.
                        
                }
                
                constexpr size_t size()const{return m_osize;}
                
                constexpr size_t allocated()const{return m_allocated;}
};


}// EON.


#endif /* SRC_POOLS_CFPOOL_H_ */
