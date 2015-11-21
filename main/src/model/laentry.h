/*
 * modelentry.h
 *      Author: Ben Cardoen
 */
#include "network/timestamp.h"
#include "model/atomicmodel.h"

#ifndef SRC_MODEL_LAENTRY_H_
#define SRC_MODEL_LAENTRY_H_

namespace n_model {

using n_network::t_timestamp;

/**
 * Stores calculated LA value for conservative.
 * @attention : reverse ordered on time : 1 > 2 == true (for max heap).
 */
class LaEntry
{
        std::size_t m_localid;
	t_timestamp m_la;
        
public:
        constexpr
	t_timestamp getTime() const
	{
		return m_la;
	}
        
        /**
         * @return The local id of the model this entry represents.
         */
        constexpr size_t getID()const
        {
                return m_localid; 
        }
       
        explicit constexpr operator size_t ()const{return getID();} 

	LaEntry() = default;
	~LaEntry() = default;
	LaEntry(const LaEntry&) = default;
	LaEntry(LaEntry&&) = default;
	LaEntry& operator=(const LaEntry&) = default;
	LaEntry& operator=(LaEntry&&) = default;

	constexpr LaEntry(std::size_t lid, t_timestamp time)
		: m_localid(lid), m_la(time)
	{
		;
	}

	friend
        constexpr
	bool operator<(const LaEntry& lhs, const LaEntry& rhs)
	{
                return (lhs.m_la > rhs.m_la); 
	}
        
        friend
        constexpr
	bool operator<=(const LaEntry& lhs, const LaEntry& rhs)
	{
		// a <= b  implies (not a>b)
		return (not (lhs > rhs));
	}

	friend
        constexpr
	bool operator>(const LaEntry& lhs, const LaEntry& rhs)
	{
		return (rhs < lhs);
	}

	friend
        constexpr
	bool operator>=(const LaEntry& lhs, const LaEntry& rhs)
	{
		return (not (lhs < rhs));
	}

	friend
        constexpr
	bool operator==(const LaEntry& lhs, const LaEntry& rhs)
	{
		return (lhs.m_localid == rhs.m_localid && lhs.getTime() == rhs.getTime());
	}
        
        friend
        constexpr
        bool operator!=(const LaEntry& lhs, const LaEntry& rhs)
        {
                return !(lhs == rhs);
        }

	friend
	std::ostream& operator<<(std::ostream& os, const LaEntry& rhs){
                return (os<<rhs.getID() << " la " << rhs.m_la);
	}
};

}

namespace std {
template<>
struct hash<n_model::LaEntry>
{
	size_t operator()(const n_model::LaEntry& item) const
	{
                return hash<std::size_t>()(item.getID());
	} 
};
}


#endif /* SRC_MODEL_LAENTRY_H_ */
