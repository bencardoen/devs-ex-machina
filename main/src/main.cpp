/*
 * This file is part of the DEVS Ex Machina project.
 * Copyright 2014 - 2015 University of Antwerp
 * https://www.uantwerpen.be/en/
 * Licensed under the EUPL V.1.1
 * A full copy of the license is in COPYING.txt, or can be found at
 * https://joinup.ec.europa.eu/community/eupl/og_page/eupl
 *      Author: Ben Cardoen, Stijn Manhaeve, Tim Tuijn
 */

#include <gtest/gtest.h>
#include "tools/globallog.h"
#include "pools/pools.h"

LOG_INIT("out.txt")



int main(int argc, char** argv)
{
	LOG_ARGV(argc, argv);
	::testing::InitGoogleTest(&argc, argv);
        n_pools::setMain();
	//gtest intercepts exceptions, else we need try/catch to force stackunwind.
	return RUN_ALL_TESTS();
}
