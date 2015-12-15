/*
 * This file is part of the DEVS Ex Machina project.
 * Copyright 2014 - 2015 University of Antwerp
 * https://www.uantwerpen.be/en/
 * Licensed under the EUPL V.1.1
 * A full copy of the license is in COPYING.txt, or can be found at
 * https://joinup.ec.europa.eu/community/eupl/og_page/eupl
 *      Author: Stijn Manhaeve, Ben Cardoen
 */

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
        constexpr uuid():m_core_id(std::numeric_limits<size_t>::max()),m_local_id(std::numeric_limits<size_t>::max()){;}
        constexpr uuid(size_t cid, size_t lid):m_core_id(cid), m_local_id(lid){;}
};

inline
std::ostream& operator<<(std::ostream& os, const uuid& id){
        os << "cid=" << id.m_core_id << " lid="<< id.m_local_id;
        return os;
}

inline constexpr
bool operator==(const uuid& l, const uuid& r)
{
	return ((l.m_core_id == r.m_core_id) && (l.m_local_id == r.m_local_id));
}
inline constexpr
bool operator<(const uuid& l, const uuid& r)
{
	return ((l.m_local_id <= r.m_local_id));
}
} 	//end namespace n_model
namespace std {
template<>
struct hash<n_model::uuid>
{
	/**
	 * @brief Hash specialization for Message. Uses Cantor's enumeration of pairs
	 */
	size_t operator()(const n_model::uuid& id) const
	{
//		const std::size_t& x = id.m_core_id;
//		const std::size_t& y = id.m_local_id;
		return id.m_local_id;
//		return std::hash<std::size_t>()(((x + y)*(x + y + 1)/2) + y);
	}
};
}	// end namespace std
#endif
