/*
 *  This file is part of cpp2json, Single statement JSON creation.
 *
 *  The MIT License (MIT)
 *
 *  Copyright (C) 2021, xyzzy@rockingship.org
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in
 *  all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 *  THE SOFTWARE.
 */

#include <iostream>
#include "cpp2json.h"

int main(int argc, char *argv[]) {

	int                      i = 42;             // numbers
	double                   d = 6.2831853071;   // floating-point
	std::string              s = "AB\"CD\007EF"; // strings
	std::vector<std::string> v;                  // vectors of ints, doubles and strings
	v.push_back("a\007b");
	v.push_back("c\"d");
	v.push_back("e\tf");

	JBASE_t blob = (JARRBEG, 1U<<31, 1.7, v, JARREND); // json blobs

	std::cout << (JOBJBEG,

            "first", JOBJBEG,
	        "number", i,
		"vector", JARRBEG, 1U << 31, "AB\"CD\007EF", v, JARREND, // arrays with mixed types
	    JOBJEND,
            "second", JOBJBEG,
	        "float", d,
	        "blob", blob,
	    JOBJEND,

	JOBJEND).toString() << "\n";

	return 0;
}
