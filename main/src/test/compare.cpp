/*
 * compare.cpp
 *
 *  Created on: Mar 12, 2015
 *      Author: Stijn Manhaeve - Devs Ex Machina
 */

#include "compare.h"

namespace n_misc{

int streamcmp(std::istream& str1, std::istream& str2){
	str1.exceptions(std::istream::failbit | std::istream::badbit);
	str2.exceptions(std::istream::failbit | std::istream::badbit);
	bool threw = false;
	//try reading a character from each stream.
	char c1 = 0;
	char c2 = 0;
	while(!str1.eof() && !str2.eof()){
		try{
			c1 = str1.get();	//this will throw on eof
		} catch (std::ios_base::failure& e){
			if (!str1.eof()) throw;
			threw = true;
		}	//still try to read from the second stream in case they both reach eof at the same time
		try{
			c2 = str2.get();	//this will throw on eof
		} catch (std::ios_base::failure& e){
			if (!str2.eof()) throw;
			threw = true;
		}
		if(threw) break;
		if(c1 != c2) return n_misc::charcmp(c1, c2);
	}

	if(str1.eof()) return str2.eof()? 0:-int((unsigned char)c2);
	return int((unsigned char)c1);
}

int filecmp(const char* file1, const char* file2){
	if(file1 == nullptr || file2 == nullptr) throw std::ios_base::failure("filecmp: filename(s) are nullptr.");
	std::ifstream stream1(file1);
	std::ifstream stream2(file2);
	return streamcmp(stream1, stream2);
}

}/*namespace misc*/
