/*
 * This file is part of the DEVS Ex Machina project.
 * Copyright 2014 - 2015 University of Antwerp
 * https://www.uantwerpen.be/en/
 * Licensed under the EUPL V.1.1
 * A full copy of the license is in COPYING.txt, or can be found at
 * https://joinup.ec.europa.eu/community/eupl/og_page/eupl
 *      Author: Stijn Manhaeve, Tim Tuijn
 */

#include "test/compare.h"

namespace n_misc{

int streamcmp(std::istream& str1, std::istream& str2, bool skipWhitespace){
	//str1.exceptions(std::istream::failbit | std::istream::badbit);
	//str2.exceptions(std::istream::failbit | std::istream::badbit);
	if(skipWhitespace){
		str1 >> std::skipws;
		str2 >> std::skipws;
	} else {
		str1 >> std::noskipws;
		str2 >> std::noskipws;
	}
	bool threw = false;
	//try reading a character from each stream.
	char c1 = 0;
	char c2 = 0;
	while(!str1.eof() && !str2.eof()){
                    str1 >> c1;	//this will throw on eof
                    if(str1.eof())
                        threw=true;
                    else 
                        if(str1.fail()){
                        throw std::ios_base::failure("Bad stream for stream 1");
                    }
                    str2 >> c2;	//this will throw on eof
                    if(str2.eof())
                        threw=true;
                    else{
                        if(str2.fail())
                            throw std::ios_base::failure("Bad stream for stream 2");
                    }
		
		if(threw) break;
		if(c1 != c2) return n_misc::charcmp(c1, c2);
	}

	if(str1.eof()) return str2.eof()? 0:-int((unsigned char)c2);
	return int((unsigned char)c1);
}

int filecmp(const char* file1, const char* file2, bool skipWhitespace){
	if(file1 == nullptr || file2 == nullptr) throw std::ios_base::failure("filecmp: filename(s) are nullptr.");
	std::ifstream stream1(file1);//, std::fstream::binary);
	std::ifstream stream2(file2);//, std::fstream::binary);
	return streamcmp(stream1, stream2, skipWhitespace);
}

}/*namespace misc*/
