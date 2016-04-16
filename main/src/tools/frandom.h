/*
 * This file is part of the DEVS Ex Machina project.
 * Copyright 2014 - 2016 University of Antwerp 
 * https://www.uantwerpen.be/en/
 * Licensed under the EUPL V.1.1
 * A full copy of the license is in COPYING.txt, or can be found at 
 * https://joinup.ec.europa.eu/community/eupl/og_page/eupl 
 *      Author: Ben Cardoen, Stijn Manhaeve
 */

/**
 * Created on 08 March 2016, 18:59
 */
#include <random>
#include <cstdlib>
#include <iostream>
#include <random>
#include <boost/random.hpp>
#include <boost/progress.hpp>
#include <thread>
#include <fstream>
#include <chrono>
#include <typeinfo>
#include <chrono>

#ifndef FRANDOM_H
#define FRANDOM_H

namespace n_tools{

/**
 * This header requires C++14 explicit support, so a todo here is to remove all constexprs if C++14 is not used, the relaxed 
 * constraints in 14 allow us to make the entire engine cexpr, in c++11 this is not possible. Need at least g++5.1 for working support.
 * To get around this, constexpr is removed where C++11 is too strict switching on the CMake set macro "-DCPP14"
 */
namespace n_frandom{
    
    
/** 
 * Compiler is smart enough even to optimize volatile away. The only 
 * resort left is external linkage, or making it global, but this is threaded testcode
 * so that would defeat the purpose. 
 * That leaves the side effect on the rng state, which should be sufficient. With constexpr rng's, it's not
 * impossible that a lot of those calls are simply resolved at c-time (but not (yet) 2e10 times).
 * TL;DR, if the below testcode finishes in less than a second, send me a link to that compiler.
*/

/**
 * Simple driver loop for an RNG.
 */
template<typename RNGTor>
void testRNG(size_t range, RNGTor rng)
{
    for(size_t i = 0; i<range; ++i){
        volatile auto r = rng();        
    }
}

/**
 * TLocal version of the above.
 */
inline
void testRNGF(size_t range, std::function<size_t(void)> rng){
    thread_local std::function<size_t(void)> gen = rng;
    for(size_t i = 0; i<range; ++i){
        volatile auto r = gen();        
    }
}

/**
 * Struct variant of the XOR shifter by G. Marsaglia. Period of 2^128-1.
 */
struct  xor128s
{
    size_t x, y, z, w, t;
    constexpr xor128s(size_t sd = 123456789):x(sd),y(362436069), z(521288629),w(88675123),t(0){;}
#ifdef CPP14
    constexpr size_t operator()(){
#else
    size_t operator()(){
#endif
        t=(x^(x<<11));
        x=y;
        y=z;
        z=w; 
        return( w=(w^(w>>19))^(t^(t>>8)) );
    }
};

/**
 * From the paper : Xorshift RNGs George Marsaglia, the xor128 shifter, period 2^128-1
 * Free function variant.
 */
inline
size_t 
xor128(){
    thread_local size_t x=123456789;
    thread_local size_t y=362436069;
    thread_local size_t z=521288629;
    thread_local size_t w=88675123;
    thread_local size_t t;
    t=(x^(x<<11));
    x=y;
    y=z;
    z=w; 
    return( w=(w^(w>>19))^(t^(t>>8)) );
}

/**
 * From the paper : Xorshift RNGs George Marsaglia, the xorwow shifter, period is 2^32
 * Adapted s.t. state is stored thread locally, which doesn't make it thread safe in all usages, but for most.
 * Free function variant.
 */
inline
size_t 
xorwow(){
    thread_local size_t x=123456789ull;
    thread_local size_t y=362436069ull;
    thread_local size_t z=521288629ull;
    thread_local size_t w=88675123ull;
    thread_local size_t v=5783321ull;
    thread_local size_t d=6615241ull;
    thread_local size_t t=(x^(x>>2)); 
    x=y; 
    y=z; 
    z=w; 
    w=v; 
    v=(v^(v<<4))^(t^(t<<1)); 
    return (d+=362437)+v;
}

/**
 * George Marsaglia's 64 XOR shifter, in an STL interface compatible version. 
 */
struct marsaglia_xor_64_s{
    typedef size_t result_type;
    constexpr size_t min()const{return 0;}
    constexpr size_t max()const{return std::numeric_limits<size_t>::max();}
    size_t _seed;

    constexpr marsaglia_xor_64_s(size_t sd = 88172645463325252ull):_seed(sd){;}

    /**
     * A seed of zero is nonsensical here, if this is passed in ignore it and use RM's default.
     */
    void seed(const size_t& sd){_seed= (sd==0) ? 88172645463325252ull : sd ;}
    
#ifdef CPP14
    constexpr size_t operator()(){
#else
    size_t operator()(){
#endif
        _seed ^= (_seed<<13);
        _seed ^= (_seed>>7);
        size_t tmp = _seed<<17;
        return (_seed^=tmp);
    }
};

/* Function variant, slower, and not easy to make it t/nt safe.*/
inline
unsigned long long int
xor64(){
    thread_local unsigned long long int seed = 88172645463325252ull;
    seed ^= (seed<<13);
    seed ^= (seed>>7);
    return (seed^=(seed<<17));
}


/**
 * Runs the usual suspects of currently best RNG's against each other in parallel.
 * Tests 2 things : speed (invariant) and t-safety, this is a threadsanitizer red flag test. It won't fail
 * gtest, but trigger any race detector if done at large enough scale if there are still issues.
 */
inline
int benchrngs() {
    
    const size_t threadcount = std::min(8u,std::thread::hardware_concurrency());
    constexpr size_t rnglimit = 200000000;
    //std::cout << "#Threadcount = " << threadcount << std::endl;
    //std::cout << "#Rng limit  = " << rnglimit << std::endl;
    std::vector<std::thread> threads;
    std::knuth_b rngknuth;
    std::mt19937_64 rngmatso;
    boost::random::taus88 rngtauss; 
    boost::random::mt11213b rngmt12;
    xor128s str;
    marsaglia_xor_64_s str2;
    //std::function<size_t(void)> x128 = [&](){return str.operator()();};
    //std::function<size_t(void)> x128 = xor128;
    std::function<size_t(void)> xwow = xorwow;
    std::function<size_t(void)> x64 = xor64;
    std::vector<std::string> dsc={{"Knuth","MT19937_64(STL)","TAUS88","MT11213b", "MarsagliaXOR128", "Marsagliaxwow", "Marsaglia64"}};
    std::vector<std::function<size_t(void)>> rngs = {{rngknuth, rngmatso, rngtauss, rngmt12, str,  xwow, str2}};
    for(size_t j = 1; j<rngs.size(); ++j){ // Skip Knuth, it simply explodes (exponential)
        std::cout << "# " << dsc[j] << std::endl;
        for(size_t q = 5; q!=0; --q ){
            std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();
            //boost::progress_timer ptime;
            for(size_t i = 0; i< threadcount; ++i){
                threads.emplace_back(std::thread( testRNGF, rnglimit>>q, rngs.at(j)));
            }
            for(auto& t : threads){
                t.join();
            }
            threads.clear();    // How about not invoking death by recalling dead threads.
            // ptime RAII's , prints collected time.
            std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>( t2 - t1 ).count();
            std::cout << (rnglimit>>q) << ",\t" <<  duration << std::endl;
        }
    }    
    return 0;
}


/**
 * Define which rng the project uses.
 */
#ifdef FRNG
    //typedef marsaglia_xor_64_s t_fastrng;
    typedef boost::random::mt11213b t_fastrng;
#else
    typedef std::mt19937_64 t_fastrng;
#endif
    
/**
 * Calculate mean/std/var online, based on Knuth's TOACP, originally by Welford.
 * See the book (v2 3rd ed p 232) for details, also on wiki and a great intro in
 * http://www.johndcook.com/blog/standard_deviation/
 */    
template<typename T>
class IncrementalStatistic{
    private:
        size_t m_cnt;
        T   m_mkmin;
        T   m_mk;
        T   m_smin;
        T   m_s;
        
    public:
        constexpr IncrementalStatistic():m_cnt(0),m_mkmin(0),m_mk(0),m_smin(0),m_s(0){;}
        
        /**
         * Taocp Book 2, 4.2.2, p 232.
         * Recurrence rel
         * m1 = arg, s1=0
         * mk = mk-1 + (arg - mk-1) / k
         * sk = sk=1 + (arg - mk-1) * (arg - mk)
         */
        // Avoid compile failures on non c++14 std. Need at least G++ 5 for void returning constexpr, and we assign member vars here
        // which is legal only in constexpr 14.
#ifdef CPP14
        constexpr void addVal(const T& v){
#else
        void addVal(const T& v){
#endif
            if(++m_cnt == 1){
                m_mkmin = v;
                m_mk = m_mkmin; // need this for 1 uninit mean
                m_smin = 0;
            }else{
                m_mk = m_mkmin  + (v - m_mkmin)/T(m_cnt);
                m_s = m_smin + (v - m_mkmin)*(v-m_mk);
                m_mkmin=m_mk;
                m_smin=m_s;
            }
        }
        
        constexpr T mean()const{
            return (T(m_cnt) < 1) ? 0 : m_mk;   // leq 1, if T fp.
        }
        
        constexpr T stddev()const {
            return std::sqrt(variance());
        }
        
        constexpr T variance()const{
            return ( m_cnt <= 1 ) ? 0: (m_s / T(m_cnt-1)); // k>=2
        }
        
        constexpr size_t count()const {return m_cnt;}
};
    
}// nfrandom 
}// ntools (Dear Santa, find me a compiler that doesn't die on a missing brace 15 compilation TU's further.)


#endif /* FRANDOM_H */

