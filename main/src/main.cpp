#include <gtest/gtest.h>
#include "ObjectFactory.h"

int
main(int argc, char** argv){
	struct S{
		S(int i){
			std::cout << "Value constructor called" << std::endl;
		}
		S(char& i){
			std::cout << "LValue constructor called" << std::endl;
		}
		~S(){std::cout << "Destructor"<< std::endl;}
	};
	auto vptr = n_tools::createObject<S>(5);
	char c = 5;
	auto lvptr = n_tools::createObject<S>(c);
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
