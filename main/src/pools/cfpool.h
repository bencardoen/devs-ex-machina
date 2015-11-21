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
#include <deque>
#include <cassert>


namespace n_pools{

/** 
 * A bitmap of sizeof<t_word>*8 bits, if a bit at MSB + i is set, then ptr
 *  at base_ptr + i is free.
 */
typedef uint64_t t_word;        // << choice is limited here to 32/64.
typedef std::vector<t_word> t_mmap;

constexpr size_t range_word = sizeof(t_word)*8;
constexpr size_t mask_word = (range_word-1);
// In future, allow dyn bset ? Speed advantage of builtins and shifting is to tempting atm.
static_assert(range_word == 64, "Static size bitset not allocated for this wordsize");


template<typename T>
struct stateful_lane{
        pool_lane<T>    m_lane;
        // Bitmap for the lane. 1 == free.
        t_mmap          m_map;
        
        /**
         * Last word where either alloc found a free pointer, or dealloc freed a pointer.
         */
        size_t          m_word_index;
        
        /**
         * @param raw A pointer (unaligned) that points to size at least sz+1 objects.
         * @param sz Lane size in (aligned) objects
         */
        stateful_lane(T* raw, size_t sz):m_lane(raw,sz),m_word_index(0)
        {
                size_t q = sz >> 6;
                size_t r = sz & mask_word;
                for(size_t i = 0; i < q; ++i)
                        m_map.push_back(std::numeric_limits<t_word>::max());
                if(r)
                        m_map.push_back(std::numeric_limits<t_word>::max()<< (range_word-r));
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
 * @attention : in simple alloc/dealloc proven to be too slow. Freelist management explodes, for small pools
 * reasonably fast but no competition for boost::pool<>.
 */
template<typename T>
class CFPool: public PoolInterface<T>
{
        private:
                // Each lane holds [begin,end] T*, and a bitmap of allocated ptrs.
                std::vector<stateful_lane<T>> m_lanes;
                // Current max size (inc allocated).
                std::size_t m_osize;
                // Total allocated objects.
                std::size_t m_allocated;
                // Lane where last free pointer was found.
                std::size_t m_lane_index;

                
                /**
                 * Create a new lane based on the current size of the pool.
                 * Update lane index to new lane.
                 * @return Ref to the new lane.
                 */
                stateful_lane<T>& 
                alloc_lane()
                {
                        LOG_DEBUG("CFPool :: allocating a new lane :: sizepre=",m_osize);
                        T* nblock = (T*) std::malloc(sizeof(T)*(m_osize+1));
                        if(!nblock)
                                throw std::bad_alloc();
                        m_lanes.push_back(stateful_lane<T>(nblock, m_osize));
                        m_osize *= 2;
                        m_lane_index=m_lanes.size()-1;
                        //LOG_DEBUG("CFPool :: allocating a new lane :: sizepost=",m_osize);
                        return m_lanes.back();
                }

        public:                
                void log_pool()
                {
#if (LOG_LEVEL != 0)
                        LOG_DEBUG("CFPool :: size=", this->size(), " alloc = ", this->allocated());
                        LOG_DEBUG("Lane index = ", m_lane_index);
                        for(const auto& lane : m_lanes){
                                LOG_DEBUG("Lane from ", lane.begin()," to ", lane.last(), " size =", lane.size());
                                LOG_DEBUG("Word index = ", lane.m_word_index);
                                for(const auto& word: lane.m_map){
                                        LOG_DEBUG(std::bitset<range_word>(word));
                                }
                        }
#endif
                }
                
                /**
                 * Set bit @pos in word.
                 * @param pos Left to right, MSB to LSB. Leftmost is 0, rightmost = sizeof*8 -1;
                 */
                void set_bit(t_word& word, size_t pos)
                {
                        assert(sizeof(word)==8 && pos < range_word && "Invalid precondition set_bit");
                        word |= (1ull << (range_word - pos - 1ull));
                        assert(word!=0 && "Logic error in set_bit");
                }
                
                // see set_bit
                void unset_bit(t_word& word, size_t pos)
                {
                        assert(sizeof(word)==8 && pos < range_word && "Invalid precondition set_bit");
                        word &= ~(1ull << (range_word - pos - 1ull));
                }
                
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
                
                /**
                 * Allocate a pointer to sizeof(T) memory, aligned @ std::alignment_of<T>::value().
                 * First fit behaviour, look for first free pointer in bitmaps, remembering it's last position.
                 * Uses the word index set by deallocate to jump to a freed bit in a word if it exists, otherwise
                 * linear search.
                 * @complexity O(N) in the worst case (alloc then dealloc)
                 */
                T* allocate()override
                {
                       
                        const size_t old_index = m_lane_index;
                        for(;;){
                                auto& lane = m_lanes[m_lane_index];
                                size_t j = lane.m_word_index;
                                for(;j<lane.m_map.size();){
                                        t_word& word = lane.m_map[j];
                                        if(word){
                                                const size_t i = n_tools::firstbitset<8>(word);        // leading zeros indicates which ptr is free
                                                T* rval = lane.begin() + (j*range_word) + i;
                                                unset_bit(word, i);
                                                ++m_allocated;
                                                lane.m_word_index=j;
                                                //LOG_DEBUG("CFPOOL:: allocated ::  ", rval , "(word) r=", j,"c=",i, " in lane ", m_lane_index);
                                                return rval;
                                        }
                                        ++j;
                                }
                                lane.m_word_index=0;
                                ++m_lane_index;
                                m_lane_index %= m_lanes.size();
                                if(m_lane_index == old_index)   
                                        break;
                        }
                        //LOG_DEBUG("CFPool :: memory exhausted, reallocating new block.");
                        auto& lane = alloc_lane();
                        T* freeptr = lane.begin();
                        ++m_allocated;
                        unset_bit(lane.m_map.front(), 0);
                        //LOG_DEBUG("CFPOOL:: allocated ::  ", freeptr, "first of new lane.", m_lane_index);
                        return freeptr;
                }
                
                /**
                 * Deallocate t.
                 * Records in the lane where the dealloc (word) occurred, so alloc can find
                 * free pointers faster should it operate in that lane.
                 * @pre t was allocated by this pool and t->~T() is not invoked yet.
                 * @post t->T() is invoked.
                 * @throws bad_alloc if t is not from this pool, logic_error if the calculation fails
                 * @complexity : -log2(N) to find the lane to which t belongs with N size of pool.
                 */
                void deallocate(T* t)override{
                        for(size_t index = 0; index<m_lanes.size();++index){
                                auto& lane = m_lanes[index];
                                if(t<lane.begin())
                                        continue;
                                uintptr_t dist = t - lane.begin(); // relies on if for oflow safety.
                                if(t<= lane.last())
                                {
                                        // %/ is signed, >= 0 since args are both unsigned
                                        const size_t quot = (dist>>6);  // dist/64
                                        //const size_t quot = dist/range_word;
                                        const size_t rem = dist & mask_word;
                                        //std::div_t rowcol = std::div(dist , range_word); // row=word, col=bit division is hideously slow here
                                        t_word& word = lane.m_map[quot];
                                        --m_allocated;
                                        set_bit(word, rem);
                                        lane.m_word_index=quot;
                                        t->~T();
                                        //LOG_DEBUG("CFPool :: deallocated ", t ," in lane ", index ," wordindex = ",quot ,"bitpos =", rem);
                                        m_lane_index = index;
                                        return;
                                }
                        }
                        //LOG_DEBUG("CFPOOL : could not dealloc :", t);
                        throw std::bad_alloc();
                }
                
                /**
                 * Max size of pool (\sum free, allocated)
                 * @return 
                 */
                constexpr size_t size()const{return m_osize;}
                
                /**
                 * Currently allocated.
                 * @return 
                 */
                constexpr size_t allocated()const{return m_allocated;}
};


}// EON.


#endif /* SRC_POOLS_CFPOOL_H_ */
