#include <iostream>

#ifndef UUID_H_
#define UUID_H_

namespace n_model{
struct uuid{
        /**
         * Core index nr.
         */
        size_t  m_core_id;
        /**
         * Local index nr.
         */
        size_t  m_local_id;
        uuid():m_core_id(0),m_local_id(0){;}
        uuid(size_t cid, size_t lid):m_core_id(cid), m_local_id(lid){;}
        friend
        std::ostream& operator<<(std::ostream&, const uuid& id);
};

inline
std::ostream& operator<<(std::ostream& os, const uuid& id){
        os << "cid=" << id.m_core_id << " lid="<< id.m_local_id;
        return os;
}




        
}//nspace
#endif