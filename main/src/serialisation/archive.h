/*
 * archive.h
 *
 *  Created on: Apr 12, 2015
 *      Author: pieter
 */

#ifndef ARCHIVE_H_
#define ARCHIVE_H_

#include "cereal/archives/binary.hpp"

namespace n_serialisation {

typedef cereal::BinaryOutputArchive t_oarchive;
typedef cereal::BinaryInputArchive t_iarchive;

}

#endif /* ARCHIVE_H_ */
