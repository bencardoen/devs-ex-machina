/*
 * This file is part of the DEVS Ex Machina project.
 * Copyright 2014 - 2015 University of Antwerp 
 * https://www.uantwerpen.be/en/
 * Licensed under the EUPL V.1.1
 * A full copy of the license is in COPYING.txt, or can be found at 
 * https://joinup.ec.europa.eu/community/eupl/og_page/eupl 
 *      Author: Stijn Manhaeve, Ben Cardoen, Tim Tuijn
 */
#include "network/timestamp.h"
#include "model/atomicmodel.h"

#ifndef SRC_MODEL_MODELENTRY_H_
#define SRC_MODEL_MODELENTRY_H_

namespace n_model {

using n_network::t_timestamp;

/**
 * Entry for a Model in a scheduler.
 * Keeps modelname and imminent time for a Model, without having to store the entire model.
 * @attention : reverse ordered on time : 1 > 2 == true (for max heap).
 */
class ModelEntry
{
        // Not const (has to go into vector)
        std::size_t m_localid;
	t_timestamp m_scheduled_at;
        
public:
        constexpr
	t_timestamp getTime() const
	{
		return m_scheduled_at;
	}
        
        /**
         * @return The local id of the model this entry represents.
         */
        constexpr size_t getID()const
        {
                return m_localid; 
        }
       
        explicit constexpr operator size_t ()const{return getID();} 

	ModelEntry() = default;
	~ModelEntry() = default;
	ModelEntry(const ModelEntry&) = default;
	ModelEntry(ModelEntry&&) = default;
	ModelEntry& operator=(const ModelEntry&) = default;
	ModelEntry& operator=(ModelEntry&&) = default;

	constexpr ModelEntry(std::size_t lid, t_timestamp time)
		: m_localid(lid), m_scheduled_at(time)
	{
		;
	}

	friend
        constexpr
	bool operator<(const ModelEntry& lhs, const ModelEntry& rhs)
	{
                return (lhs.m_scheduled_at == rhs.m_scheduled_at)? (lhs.m_localid > rhs.m_localid):(lhs.m_scheduled_at > rhs.m_scheduled_at); 
	}
        
        friend
        constexpr
	bool operator<=(const ModelEntry& lhs, const ModelEntry& rhs)
	{
		// a <= b  implies (not a>b)
		return (not (lhs > rhs));
	}

	friend
        constexpr
	bool operator>(const ModelEntry& lhs, const ModelEntry& rhs)
	{
		return (rhs < lhs);
	}

	friend
        constexpr
	bool operator>=(const ModelEntry& lhs, const ModelEntry& rhs)
	{
		return (not (lhs < rhs));
	}

	friend
        constexpr
	bool operator==(const ModelEntry& lhs, const ModelEntry& rhs)
	{
		return (lhs.m_localid == rhs.m_localid /*&& lhs.m_scheduled_at == rhs.m_scheduled_at*/); // uncomment to allow multiple entries per model
	}
        
        friend
        constexpr
        bool operator!=(const ModelEntry& lhs, const ModelEntry& rhs)
        {
                return !(lhs == rhs);
        }

	friend
	std::ostream& operator<<(std::ostream& os, const ModelEntry& rhs){
                return (os<<rhs.getID() << " scheduled at " << rhs.m_scheduled_at);
	}
};

}

namespace std {
template<>
struct hash<n_model::ModelEntry>
{
	size_t operator()(const n_model::ModelEntry& item) const
	{
                return hash<std::size_t>()(item.getID());
	}
        
        
};
}

namespace n_model{

template<typename Object>
class IntrusiveEntry{
private:
        Object*         m_modelp;
        size_t          m_lid;
public:
        explicit IntrusiveEntry(Object* m):m_modelp(m),m_lid(m->getLocalID()){;}
        IntrusiveEntry()=default;
        ~IntrusiveEntry()=default;
        
        explicit operator size_t ()const{
                return m_lid;
        }
        
        explicit operator t_timestamp () const{
                return m_modelp->getTimeNext();
        }
        

	friend
	bool operator<(const IntrusiveEntry& lhs, const IntrusiveEntry& rhs)
	{
                return (t_timestamp (lhs) == t_timestamp (rhs))? (size_t (lhs) > size_t (rhs)):(t_timestamp (lhs) > t_timestamp (rhs));          
	}
        
        friend
	bool operator<=(const IntrusiveEntry& lhs, const IntrusiveEntry& rhs)
	{
		// a <= b  implies (not a>b)
		return (not (lhs > rhs));
	}

	friend
	bool operator>(const IntrusiveEntry& lhs, const IntrusiveEntry& rhs)
	{
		return (rhs < lhs);
	}

	friend
	bool operator>=(const IntrusiveEntry& lhs, const IntrusiveEntry& rhs)
	{
		return (not (lhs < rhs));
	}

	friend
	bool operator==(const IntrusiveEntry& lhs, const IntrusiveEntry& rhs)
	{
		return (size_t(lhs) == size_t(rhs));
	}

};

}

#endif /* SRC_MODEL_MODELENTRY_H_ */
