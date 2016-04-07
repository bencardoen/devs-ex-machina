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
#include <typeinfo>
#include <chrono>

#ifndef FRANDOM_H
#define FRANDOM_H

namespace n_tools{

namespace n_frandom{
    
using namespace std;

template<typename RNGTor>
void testRNG(size_t range, RNGTor rng)
{
    for(size_t i = 0; i<range; ++i){
        volatile auto r = rng();        // Volatile since we don't use this value, if the RNG has no observable side effects, the compiler can just remove this for loop.
    }
}

void testRNGF(size_t range, std::function<size_t(void)> rng){
    thread_local std::function<size_t(void)> gen = rng;
    for(size_t i = 0; i<range; ++i){
        volatile auto r = gen();        // Volatile since we don't use this value, if the RNG has no observable side effects, the compiler can just remove this for loop.
    }
}

/**
 * Struct variant of the XOR shifter by G. Marsaglia. Period of 2^128-1.
 */
struct  xor128s
{
    size_t x, y, z, w, t;
    constexpr xor128s(size_t sd = 123456789):x(sd),y(362436069), z(521288629),w(88675123),t(0){;}
    
    constexpr size_t operator()(){
        t=(x^(x<<11));
        x=y;
        y=z;
        z=w; 
        return( w=(w^(w>>19))^(t^(t>>8)) );
    }
};

/**
 * From the paper : Xorshift RNGs George Marsaglia, the xor128 shifter, period 2^128-1
 */
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
 */
size_t 
xorwow(){
    thread_local size_t x=123456789;
    thread_local size_t y=362436069;
    thread_local size_t z=521288629;
    thread_local size_t w=88675123;
    thread_local size_t v=5783321;
    thread_local size_t d=6615241;
    thread_local size_t t=(x^(x>>2)); 
    x=y; 
    y=z; 
    z=w; 
    w=v; 
    v=(v^(v<<4))^(t^(t<<1)); 
    return (d+=362437)+v;
}

/**
 * George Marsaglia's 64 XOR shifter.
 */
struct xor64s{
    typedef size_t result_type;
    constexpr size_t min()const{return 0;}
    constexpr size_t max()const{return std::numeric_limits<size_t>::max();}
    size_t _seed;
    constexpr xor64s(size_t sd = 88172645463325252ll):_seed(sd){;}
    void seed(const size_t& sd){_seed=sd;}
    constexpr size_t operator()(){
        _seed ^= (_seed<<13);
        _seed ^= (_seed>>7);
        return (_seed^=(_seed<<17));
    }
};

/* Function variant, slower.*/
unsigned long long int
xor64(){
    thread_local unsigned long long int seed = 88172645463325252ll;
    seed ^= (seed<<13);
    seed ^= (seed>>7);
    return (seed^=(seed<<17));
}


/*
 * 
 */
int benchrngs() {
    const size_t threadcount = std::thread::hardware_concurrency();
    constexpr size_t rnglimit = 200000000;
    std::cout << "#Threadcount = " << threadcount << std::endl;
    std::cout << "#Rng limit  = " << rnglimit << std::endl;
    std::vector<std::thread> threads;
    knuth_b rngknuth;
    mt19937_64 rngmatso;
    boost::random::taus88 rngtauss; 
    boost::random::mt11213b rngmt12;
    xor128s str;
    xor64s str2;
    //std::function<size_t(void)> x128 = [&](){return str.operator()();};
    //std::function<size_t(void)> x128 = xor128;
    std::function<size_t(void)> xwow = xorwow;
    std::function<size_t(void)> x64 = xor64;
    std::vector<std::string> dsc={{"Knuth","MT19937_64(STL)","TAUS88","MT11213b", "MarsagliaXOR128", "Marsagliaxwow", "Marsaglia64"}};
    std::vector<std::function<size_t(void)>> rngs = {{rngknuth, rngmatso, rngtauss, rngmt12, str,  xwow, str2}};
    for(size_t j = 1; j<rngs.size(); ++j){ // Skip Knuth, it simply explodes
        std::cout << "# " << dsc[j] << std::endl;
        for(size_t q = 5; q!=0; --q ){
            //chrono::high_resolution_clock::time_point t1 = chrono::high_resolution_clock::now();
            boost::progress_timer ptime;
            for(size_t i = 0; i< threadcount; ++i){
                threads.emplace_back(std::thread( testRNGF, rnglimit>>q, rngs.at(j)));
            }
            for(auto& t : threads){
                t.join();
            }
            threads.clear();    // How about not invoking death by recalling dead threads.
            // ptime RAII's , prints collected time.
            //chrono::high_resolution_clock::time_point t2 = chrono::high_resolution_clock::now();
            //auto duration = chrono::duration_cast<chrono::milliseconds>( t2 - t1 ).count();
            //std::cout << (rnglimit>>q) << ",\t" <<  duration << std::endl;
        }
    }
    
    std::uniform_int_distribution<> uid(1,10);
    uid(str2);
    str2.seed(0);
    uid(str2);
    return 0;
}
    
}// nfrandom 
}// ntools (Dear Santa, find me a compiler that doesn't die on a missing brace 15 compilation TU's further.)


#endif /* FRANDOM_H */

