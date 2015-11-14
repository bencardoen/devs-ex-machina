#include <iostream>
#include <set>
#include <map>
#include <sstream>
#include "tools/globallog.h"
#include "pools/pools.h"


using namespace n_pools;

struct __attribute__((aligned(64)))Msg{
        size_t v;
        Msg():v(0){;}
        explicit Msg(size_t iv):v(iv){;}
};


// Todo : the below functions could be folded into one and passed the pool as argument, with the exception of object_pool which then loses speed.
void testPool(size_t testsize, size_t rounds, PoolInterface<Msg>* pool){
        std::vector<Msg*> ptrs;
        for(size_t r = 0; r<rounds; ++r){
                for(size_t t = 0; t<testsize; ++t){
                        Msg* ptr = new ( pool->allocate() ) Msg(142);
                        ptr->v+=1;
                        ptrs.push_back(ptr);
                }
                for(auto p : ptrs)
                        pool->deallocate(p);
                ptrs.clear();
        }
}

int main(int argc, char** argv) {
        if(argc != 4){
                std::cout << "Usage : objectcount rounds pooltype" << std::endl;
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
        std::map<std::string, PoolInterface<Msg>*>  pnames{     
                                                        {"bpool",new n_pools::Pool<Msg, boost::pool<>>(128)},{"slabpool",new n_pools::SlabPool<Msg>(tsize)},
                                                        {"dynpool",new n_pools::DynamicSlabPool<Msg>(128)} ,{"stackpool", new n_pools::StackPool<Msg>(128)},
                                                        {"newdel", new n_pools::Pool<Msg, std::false_type>(tsize/2)}
                                                          };
        //size_t allocsize = (tsize*sizeof(Msg))/(1024*1024);
        //std::cout << "Benchmark will allocate at least " << allocsize << " MB " << std::endl;
        const auto& found = pnames.find(ptype);
        if(found==pnames.end()){
                std::cout << "Failed to find pooltype" << std::endl;
                return -1;
        }else{
                testPool(tsize, r,found->second);
        }
        for(auto& n_p : pnames)
                delete n_p.second;
                
}
