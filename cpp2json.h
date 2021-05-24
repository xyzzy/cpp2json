#ifndef _CPP2JSON_H
#define _CPP2JSON_H

/*
 *  Use it like it's javascript.
 *
 * -------------
 * (JOBJ, "meta", JOBJ,
 *			    "start", ustart,
 *			    "vector", JARR, 42, 6.2831853071,  "ABCD\"EFGHI", vector, JARREND,
 *		      JOBJEND,
 *	      "data", JOBJ,
 *			    "start", dstart,
 *			    "vector", vector,
 *		      JOBJEND,
 *	      "pow31", 1U<<31,
 * JOBJEND)
 * -------------
 *
 *  Replace:
 *    "[" with JARRBEG
 *    "]" with JARREND
 *    "{" with JOBJBEG
 *    "}" with JOBJEND
 *
 *  Values can be: ints, doubles, strings, vector<int>, vector<double>, vector<string>
 */

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

#include <cassert>
#include <iostream>
#include <string>
#include <vector>
#include <stdio.h>
#include <stdint.h>

/*
 * Leaf wrappers
 */
struct JLEAF_t {

	// type of content
	typedef enum {
		// types with data stored in union
		NUMBER, DOUBLE, STRING, OBJECT, ARRAY, PAIR,
		// placeholders that later get converted to OBJECT/ARRAY
		OBJBEG, OBJEND, ARRBEG, ARREND
	} typ_t;

	typ_t typ;

	/*
	 * only one is used to share save address space
	 *
	 * @date 2021-05-24 02:06:37
	 *
	 * `number` is limited to 32-bit signed, this is the most safe across all implementations
	 * Larger numbers are possible and will be exported as strings.
	 * When loading JSON from javascript expect large numbers to unsigned numbers , prepare for string-to-integer conversion.
	 */
	union {
		int                             i;  // NUMBER and placeholders
		double                          d;  // DOUBLE
		std::string                     *s; // STRING
		std::vector<JLEAF_t>            *v; // OBJECT/ARRAY
		std::pair<std::string, JLEAF_t> *p; // PAIR
	};

	/*
	 * Default constructor
	 */
	JLEAF_t() : typ(NUMBER), i(0) {}

	/*
	 * Construct a placeholder
	 */
	JLEAF_t(typ_t typ) : typ(typ), i(0) {}

	/*
	 * Construct a 32-bit signed number
	 */
	JLEAF_t(const int i) : typ(NUMBER), i(i) {}

	/*
	 * Construct a 32-bit unsigned number
	 */
	JLEAF_t(const unsigned i) : typ(NUMBER), i((int) i) {
		// is value in the safe range
		if (i & 0x80000000) {
			/*
			 * Convert number to string
			 * Receiving end: `std::string toString(void)`
			 */
			char sbuf[32];
			sprintf(sbuf, "%u", i);
			typ = STRING;
			s   = new std::string(sbuf);
		}
	}

	/*
	 * Construct a larger number, possibly exporting as string.
	 */
	JLEAF_t(const int64_t i) : typ(NUMBER), i((int) i) {
		// is value in the safe range
		if (i & -0x80000000LL) {
			/*
			 * Convert number to string
			 * Receiving end: `std::string toString(void)`
			 */
			char sbuf[32];
			assert(sizeof i == 8);
			sprintf(sbuf, "%ld", i);
			typ = STRING;
			s   = new std::string(sbuf);
		}
	}

	/*
	 * Construct a larger number, possibly exporting as string.
	 */
	JLEAF_t(const uint64_t i) : typ(NUMBER), i((int) i) {
		// is value in the safe range
		if (i & -0x80000000LL) {
			/*
			 * Convert number to string
			 * Receiving end: `std::string toString(void)`
			 */
			char sbuf[32];
			assert(sizeof i == 8);
			sprintf(sbuf, "%lu", i);
			typ = STRING;
			s   = new std::string(sbuf);
		}
	}

	/*
	 * Construct a double
	 */
	JLEAF_t(const double d) : typ(DOUBLE), d(d) {}

	/*
	 * Construct a string
	 */
	JLEAF_t(const std::string &s) : typ(STRING), s(new std::string(s)) {}

	/*
	 * Construct a name+value pair
	 */
	JLEAF_t(const std::string &name, const JLEAF_t &value) : typ(PAIR), p(new std::pair<std::string, JLEAF_t>(name, value)) {}

	/*
	 * Construct an object/array vector
	 */
	JLEAF_t(typ_t typ, std::vector<JLEAF_t> &v) : typ(typ), v(&v) {}

	/*
	 * Convert contents to text
	 */
	std::string toString(void) {
		switch (typ) {
		case NUMBER: {
			char sbuf[32];
			sprintf(sbuf, "%d", i);
			return std::string(sbuf);
		}
		case DOUBLE: {
			char sbuf[32];
			sprintf(sbuf, "%.15g", d);
			return std::string(sbuf);
		}
		case STRING:
			return '"' + *s + '"';
		case OBJECT: {
			std::string sbuf;

			if (v->empty())
				return "{}";

			// expand object elements
			for (unsigned i = 0; i < v->size(); i++) {
				// header/delimiter
				if (i)
					sbuf += ',';
				else
					sbuf += '{';

				// element
				sbuf += (*v)[i].toString();
			}
			// trailer
			sbuf += '}';
			return sbuf;
		}
		case ARRAY: {
			std::string sbuf;

			if (v->empty())
				return "[]";

			// expand array elements
			for (unsigned i = 0; i < v->size(); i++) {
				// header/delimiter
				if (i)
					sbuf += ',';
				else
					sbuf += '[';

				// element
				sbuf += (*v)[i].toString();
			}
			// trailer
			sbuf += ']';
			return sbuf;
		}
		case PAIR: {
			std::string sbuf;

			sbuf += '"' + p->first + "\":";
			sbuf += p->second.toString();
			return sbuf;
		}
		case OBJBEG:
			return "#OBJBEG#";
		case OBJEND:
			return "#OBJEND#";
		case ARRBEG:
			return "#ARRBEG#";
		case ARREND:
			return "#ARREND#";
		default:
			return "#UNKNOWN";
		}
	}

	/*
	 * Assign resources
	 */
	void assign(const JLEAF_t &rhs) {
		this->typ = rhs.typ;
		switch (typ) {
		case NUMBER:
			this->i = rhs.i;
			break;
		case DOUBLE:
			this->d = rhs.d;
			break;
		case STRING:
			this->s = new std::string(*rhs.s);
			break;
		case OBJECT:
		case ARRAY:
			this->v = new std::vector<JLEAF_t>(*rhs.v);
			break;
		case PAIR:
			this->p = new std::pair<std::string, JLEAF_t>(*rhs.p);
			break;
		default:
			// placeholders
			this->i = 0;
			break;
		}
	}

	/*
	 * Release resources
	 */
	void release(void) {
		switch (typ) {
		case STRING:
			delete s;
			s = NULL;
			break;
		case OBJECT:
		case ARRAY:
			delete v;
			v = NULL;
			break;
		case PAIR:
			delete p;
			p = NULL;
			break;
		default:
			break;
		}
	}

	/*
	 * Copy constructor
	 */
	JLEAF_t(const JLEAF_t &rhs) {
		assign(rhs);
	}

	/*
	 * assignment operator
	 */
	JLEAF_t &operator=(const JLEAF_t &rhs) {
		release();
		assign(rhs);
		return *this;
	}

	/*
	 * destructor
	 */
	~JLEAF_t() {
		release();
	}
};

/*
 * Base class to collect json elements
 */
struct JBASE_t {
	std::vector<JLEAF_t> root;

	/*
	 * Convert contents to text
	 */
	std::string toString(void) {
		assert(root.size() == 1);
		return root[0].toString();
	}

	/*
	 * Escape a JSON string
	 */
	static std::string escapeString(const std::string &s) {

		std::string sbuf;

		for (unsigned i = 0; i < s.length(); i++) {
			unsigned char ch = s[i];
			switch (ch) {
			case '"':
				sbuf += "\\\"";
				break;
			case '\\':
				sbuf += "\\\\";
				break;
			case '/':
				sbuf += "\\/";
				break;
			case '\b':
				sbuf += "\\b";
				break;
			case '\f':
				sbuf += "\\f";
				break;
			case '\n':
				sbuf += "\\n";
				break;
			case '\r':
				sbuf += "\\r";
				break;
			case '\t':
				sbuf += "\\t";
				break;
			default:
				if (iscntrl(ch)) {
					sbuf += "\\u00";
					sbuf += "0123456789abcdef"[ch >> 4];
					sbuf += "0123456789abcdef"[ch & 0xf];
				} else {
					sbuf += ch;
				}
			}
		}

		return sbuf;
	}
};

/*
 * Chain starts with the equivalent of "{"
 */
struct JOBJBEG_t : JBASE_t {
	/*
	 * Constructor
	 */
	JOBJBEG_t() {
		/*
		 * Push first leaf
		 */
		root.push_back(JLEAF_t(JLEAF_t::OBJBEG));
	}
};

/*
 * Chain starts with the equivalent of "}"
 * NOTE: This might happen when chains are evaluated right-to-left
 */
struct JOBJEND_t : JBASE_t {
	/*
	 * Constructor
	 */
	JOBJEND_t() {
		/*
		 * Push first leaf
		 */
		root.push_back(JLEAF_t(JLEAF_t::OBJEND));
	}
};

/*
 * Chain starts with the equivalent of "["
 */
struct JARRBEG_t : JBASE_t {
	/*
	 * Constructor
	 */
	JARRBEG_t() {
		/*
		 * Push first leaf
		 */
		root.push_back(JLEAF_t(JLEAF_t::ARRBEG));
	}
};

/*
 * Chain starts with the equivalent of "]"
 * NOTE: This might happen when cascades are evaluated right-to-left
 */
struct JARREND_t : JBASE_t {
	/*
	 * Constructor
	 */
	JARREND_t() {
		/*
		 * Push first leaf
		 */
		root.push_back(JLEAF_t(JLEAF_t::ARREND));
	}
};

/*
 * Concatenate a 32-bit signed number
 */
JBASE_t operator,(JBASE_t r, const int rhs) {
	r.root.push_back(JLEAF_t(rhs));
	return r;
}

/*
 * Concatenate a 32-bit unsigned number
 */
JBASE_t operator,(JBASE_t r, const unsigned rhs) {
	r.root.push_back(JLEAF_t(rhs));
	return r;
}

/*
 * Concatenate an unsigned number
 */
JBASE_t operator,(JBASE_t r, const uint64_t rhs) {
	r.root.push_back(JLEAF_t(rhs));
	return r;
}
/*
 * Concatenate a double floating point
 */
JBASE_t operator,(JBASE_t r, const double rhs) {
	r.root.push_back(JLEAF_t(rhs));
	return r;
}

/*
 * Concatenate a string
 */
JBASE_t operator,(JBASE_t r, const std::string &rhs) {
	/*
	 * Escape special chars
	 */
	const std::string &escaped = JBASE_t::escapeString(rhs);

	r.root.push_back(JLEAF_t(escaped));
	return r;
}

/*
 * Concatenate a vector of ints
 */
JBASE_t operator,(JBASE_t r, const std::vector<int> &rhs) {
	// copy elements to vector
	std::vector<JLEAF_t> *arr = new std::vector<JLEAF_t>();

	for (unsigned i = 0; i < rhs.size(); i++)
		arr->push_back(JLEAF_t(rhs[i]));

	r.root.push_back(JLEAF_t(JLEAF_t::ARRAY, *arr));
	return r;
}

/*
 * Concatenate a vector of doubles
 */
JBASE_t operator,(JBASE_t r, const std::vector<double> &rhs) {
	// copy elements to vector
	std::vector<JLEAF_t> *arr = new std::vector<JLEAF_t>();

	for (unsigned i = 0; i < rhs.size(); i++)
		arr->push_back(JLEAF_t(rhs[i]));

	r.root.push_back(JLEAF_t(JLEAF_t::ARRAY, *arr));
	return r;
}

/*
 * Concatenate a vector of strings
 */
JBASE_t operator,(JBASE_t r, const std::vector<std::string> &rhs) {
	// copy elements to vector
	std::vector<JLEAF_t> *arr = new std::vector<JLEAF_t>();

	for (unsigned i = 0; i < rhs.size(); i++) {
		const std::string &escaped = JBASE_t::escapeString(rhs[i]);
		arr->push_back(JLEAF_t(escaped));
	}

	r.root.push_back(JLEAF_t(JLEAF_t::ARRAY, *arr));
	return r;
}

/*
 * Concatenate a vector of strings
 */
JBASE_t operator,(JBASE_t r, const std::vector<const char *> &rhs) {
	// copy elements to vector
	std::vector<JLEAF_t> *arr = new std::vector<JLEAF_t>();

	for (unsigned i = 0; i < rhs.size(); i++) {
		const std::string &escaped = JBASE_t::escapeString(rhs[i]);
		arr->push_back(JLEAF_t(escaped));
	}

	r.root.push_back(JLEAF_t(JLEAF_t::ARRAY, *arr));
	return r;
}

/*
 * Basic concatenation of existing blob
 */
JBASE_t operator,(JBASE_t r, const JBASE_t &rhs) {
	// push contents
	for (unsigned i = 0; i < rhs.root.size(); i++)
		r.root.push_back(rhs.root[i]);
	return r;
}

/*
 * Concatenate start-of-object
 */
JBASE_t operator,(JBASE_t r, const JOBJBEG_t &rhs) {
	// push indicator
	r.root.push_back(JLEAF_t(JLEAF_t::OBJBEG));

	return r;
}

/*
 * Concatenate end-of-object.
 * Gather all pairs starting from start-of-object and bundle them into a single leaf
 */
JBASE_t operator,(JBASE_t r, const JOBJEND_t &rhs) {
	// how many elements in root structure
	unsigned size = r.root.size();
	// must be something
	assert(size);

	// walkback to find the matching `JOBJ`
	unsigned start = size - 1;
	while (start > 0 && r.root[start].typ != JLEAF_t::OBJBEG)
		--start;

	// must be present
	if (r.root[start].typ != JLEAF_t::OBJBEG) {
		std::cerr << "OBJBEG not found on stack:\n";
		for (unsigned i = 0; i < r.root.size(); i++)
			std::cerr << i << ": " << r.root[i].toString() << "\n";
		assert(r.root[start].typ == JLEAF_t::OBJBEG);
	}

	// copy elements to vector
	std::vector<JLEAF_t> *v = new std::vector<JLEAF_t>();

	for (unsigned i = start + 1; i < size; i += 2) {
		assert(r.root[i].typ == JLEAF_t::STRING);

		v->push_back(JLEAF_t(*r.root[i].s, r.root[i + 1]));
	}

	// make `v` the last entry
	r.root[start] = JLEAF_t(JLEAF_t::OBJECT, *v);
	r.root.resize(start + 1);

	return r;
}

/*
 * Concatenate start-of-array
 */
JBASE_t operator,(JBASE_t r, const JARRBEG_t &rhs) {
	// push indicator
	r.root.push_back(JLEAF_t(JLEAF_t::ARRBEG));

	return r;
}

/*
 * Concatenate end-of-array.
 * Gather all elements starting from start-of-array and bundle them into a single leaf
 */
JBASE_t operator,(JBASE_t r, const JARREND_t &rhs) {
	// how many elements in root structure
	unsigned size = r.root.size();
	// must be something
	assert(size);

	// walkback to find the matching `JARR`
	unsigned start = size - 1;
	while (start > 0 && r.root[start].typ != JLEAF_t::ARRBEG)
		--start;

	// must be present
	if (r.root[start].typ != JLEAF_t::ARRBEG) {
		std::cerr << "ARRBEG not found on stack:\n";
		for (unsigned i = 0; i < r.root.size(); i++)
			std::cerr << i << ": " << r.root[i].toString() << "\n";
		assert(r.root[start].typ == JLEAF_t::ARRBEG);
	}

	// copy elements to vector
	std::vector<JLEAF_t> *v = new std::vector<JLEAF_t>();

	for (unsigned i = start + 1; i < size; i++)
		v->push_back(r.root[i]);

	// make `v` the last entry
	r.root[start] = JLEAF_t(JLEAF_t::ARRAY, *v);
	r.root.resize(start + 1);

	return r;
}

/*
 * Reserved words
 */
JOBJBEG_t JOBJBEG;
JOBJEND_t JOBJEND;
JARRBEG_t JARRBEG;
JARREND_t JARREND;

#endif // _CPP2JSON_H
