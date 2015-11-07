/**
 * @author Ben Cardoen -- devs ex machina
 */

#ifndef MID_H_
#define MID_H_

#include <cstdint>

namespace n_network{

namespace n_const{
        /**
        * The constants defined in this namespace constrict the unique identifiers of ports/models/cores.
        * Packed together they form in the struct m(essage)id(entifier) a unique id for a port/model in a core. 
        * If you're reaching one of the above limits, it's possible to redefine these constants to increase/decrease ranges.
        * Don't ever break the constraint : range(storage) >= sum(range(parts)).
        */
        // Unit = bit.
        // Ranges should sum to this field's bitsize.
        constexpr size_t range_word = 64;
        
        // Port
        constexpr size_t range_port = 16;
        constexpr size_t offset_port = range_word-range_port;
        /**
         * Maximum usable identifier for a port.
         */
        constexpr size_t port_max = (1ULL<<range_port)-1;
        
        // Core
        constexpr size_t range_core = 8;
        constexpr size_t offset_core = offset_port - range_core;
        /**
         * Maximum usable identifier for a core.
         */
        constexpr size_t core_max = (1ULL<<range_core)-1;
        
        // Model
        constexpr size_t range_model = range_word-(range_port+range_core);
        /**
         * Maximum usable identifier for a model.
         */
        constexpr size_t model_max = (1ULL<<range_model)-1;
        
        static_assert(range_port + range_model + range_core <= range_word, "SUm ranges bitfields > allocated size.");
}
 typedef uint64_t                t_word;
/**
 * Message identifier. Uniquely references port/core/model instance.
 * @attention : attributes are not required for the moment, they will be if the user starts changing the sizes of the mapped fields. 
 * Bitfields were avoided for a nr of reasons, the first being loss of control over allocation size. In the current implementation, the 
 * whole field fits into a word/register for maximal speed.
 */
struct __attribute__((packed)) mid{
private:
        t_word  m_storage;
        // Masks for each individual identifier. 1 for each corresponding bitfield position, 0 otherwise.

        static constexpr size_t pmask = ((1ULL << n_const::range_port)-1)<<n_const::offset_port;
        static constexpr size_t cmask = ((1ULL << n_const::range_core)-1)<<n_const::offset_core;
        static constexpr size_t mmask = ((1ULL << n_const::range_model)-1);
public:
        constexpr mid():m_storage(0){;}
        /**
         * Unchecked constructor. If any value exceeds the allocated range, shift overflow will occur.
         */
        constexpr mid(size_t port, size_t core, size_t modelid):m_storage( (port<<n_const::offset_port) | (core<<n_const::offset_core) | modelid )
        {
                ;
        }
        // Destructor and 4 friends are well defined for POD.
        
        void zeroAll()
        {
                m_storage=0;
        }
        
        constexpr size_t portid()const
        {
                return ((m_storage & pmask)>>n_const::offset_port);
        }
        
        constexpr size_t coreid()const
        {
                return ((m_storage & cmask)>>n_const::offset_core);
        }
        
        constexpr size_t modelid()const
        {
                return (m_storage & mmask);
        }
        
        void setportid(size_t pid)
        {
#ifdef SAFETY_CHECKS
                if(pid > pmask)
                        throw std::out_of_range("Port id out of range.");
#endif
                m_storage = (~pmask & m_storage) | (pid << n_const::offset_port);  //*
        }
        
        void setcoreid(size_t cid)
        {
#ifdef SAFETY_CHECKS
                if(cid > cmask)
                        throw std::out_of_range("Core id out of range.");
#endif
                m_storage = (~cmask & m_storage) | (cid << n_const::offset_core); 
        }
        
        void setmodelid(size_t modelid)
        {
#ifdef SAFETY_CHECKS
                if(modelid > mmask)
                        throw std::out_of_range("Model id out of range.");
#endif
                m_storage = (~mmask & m_storage) | (modelid); 
        }
        
        size_t getField()const{return m_storage;}
        
        constexpr bool operator==(const mid& rhs)const
        {
                return m_storage==rhs.m_storage;
        }
        constexpr bool operator<(const mid& rhs)const
        {
                return m_storage<rhs.m_storage;
        }
};

}

namespace std {
template<>
struct hash<n_network::mid>
{
	size_t operator()(const n_network::mid& id) const
	{
		return id.getField();// unique by its semantics, so don't hash.
	}
};
}	// end namespace std
#endif
