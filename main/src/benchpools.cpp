#include <iostream>
#include <set>
#include <map>
#include <sstream>
#include "tools/pools.h"
#include "network/message.h"
#include "model/uuid.h"

using n_network::Message;
using n_model::uuid;
using n_network::t_timestamp;
using namespace n_tools;

// Todo : the below functions could be folded into one and passed the pool as argument, with the exception of object_pool which then loses speed.
void testOPool(size_t testsize, size_t rounds){
        std::vector<Message*> mptrs(testsize);
        for(size_t j = 0; j<rounds; ++j){
                Pool<Message, boost::object_pool<Message>> pl(testsize);
                for(size_t i = 0; i<testsize; ++i){
                        Message* rawmem = pl.allocate();
                        Message * msgconstructed = new (rawmem) Message( uuid(1,1), uuid(2,2), t_timestamp(3,4), 5, 6);
                        //msgconstructed->setAntiMessage(true);
                        mptrs[i]=msgconstructed;
                }
                mptrs.clear();
        }
}

void testDPool(size_t testsize, size_t rounds){
        std::vector<Message*> mptrs(testsize);
        Pool<Message, std::false_type> pl(testsize);
        for(size_t j = 0; j<rounds;++j){
                for(size_t i = 0; i<testsize; ++i){
                        Message* rmem = pl.allocate();
                        Message * msgconstructed = new (rmem) Message( uuid(1,1), uuid(2,2), t_timestamp(3,4), 5, 6);
                        //msgconstructed->setAntiMessage(true);
                        mptrs[i]=msgconstructed;
                }
                
                for(auto p : mptrs){
                        pl.deallocate(p);
                }
                
        }
}


void testPool(size_t testsize, size_t rounds){
        std::vector<Message*> mptrs(testsize);
        // Allocate Message sized objects, with an initial grab of testsize objects from OS.
        Pool<Message, boost::pool<>> pl(testsize); 
        for(size_t j = 0; j<rounds;++j){
                for(size_t i = 0; i<testsize; ++i){
                        Message * rawmem = (Message*) pl.allocate();
                        Message * msgconstructed = new(rawmem) Message( uuid(1,1), uuid(2,2), t_timestamp(3,4), 5, 6);
                        //msgconstructed->setAntiMessage(true);
                        mptrs[i]=msgconstructed;
                }
        
                for(auto p : mptrs){
                        pl.deallocate(p);
                }
                
        }
}

void testTSPool(size_t testsize, size_t rounds){
        std::vector<Message*> mptrs(testsize);
        // Allocate Message sized objects, with an initial grab of testsize objects from OS.
        Pool<Message, spool<Message>> pl(0,0); 
        for(size_t j = 0; j<rounds;++j){
                for(size_t i = 0; i<testsize; ++i){
                        Message * rawmem = (Message*) pl.allocate();
                        Message * msgconstructed = new(rawmem) Message( uuid(1,1), uuid(2,2), t_timestamp(3,4), 5, 6);
                        //msgconstructed->setAntiMessage(true);
                        mptrs[i]=msgconstructed;
                }
        
                for(auto p : mptrs){
                        pl.deallocate(p);
                }
                
        }
}



void testSlabPool(size_t testsize, size_t rounds){
        Pool<Message, SlabPool<Message>> pl(testsize);
        std::vector<Message*> mptrs(testsize);
        for(size_t j = 0; j<rounds;++j){
                for(size_t i = 0; i<testsize; ++i){
                        Message* rawmem = pl.allocate();
                        Message * msgconstructed = new(rawmem) Message( uuid(1,1), uuid(2,2), t_timestamp(3,4), 5, 6);
                        //msgconstructed->setAntiMessage(true);
                        mptrs[i]=msgconstructed;
                }
                for(auto p : mptrs){
                        pl.deallocate(p);
                }
        }
}

int main(int argc, char** argv) {
        std::map<std::string, std::function<void(size_t, size_t)>>  pnames{    {"nopool", testDPool}, {"tpool", testTSPool}, 
                                                                                {"slabpool", testSlabPool}, {"opool",testOPool},
                                                                                {"rpool",testPool} };
        if(argc != 4){
                std::cout << "Usage : objectcount rounds pooltype" << std::endl;
                std::cout << "where pooltype is one of " << std::endl;
                for(const auto& k : pnames)
                        std::cout << k.first << ", ";
                std::cout << std::endl;
                return 0;
        }
        std::string testsize(argv[1]);
        std::stringstream s;
        size_t tsize = 0;
        s << testsize;
        s >> tsize;
        std::string testrounds(argv[2]);
        size_t r = 0;
        s.clear();
        s << testrounds;
        s >> r;
        std::string ptype(argv[3]);
        size_t allocsize = (tsize*sizeof(Message))/(1024*1024);
        std::cout << "Benchmark will allocate at least " << allocsize << " MB " << std::endl;
        const auto& found = pnames.find(ptype);
        if(found==pnames.end()){
                std::cout << "Failed to find pooltype" << std::endl;
                return -1;
        }else{
                found->second(tsize, r);
        }
                
}
