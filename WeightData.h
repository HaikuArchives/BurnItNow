/*
 * Copyright 2012 Tri-Edge AI <triedgeai@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#ifndef _WEIGHT_DATA_H_
#define _WEIGHT_DATA_H_

#include <fstream>
#include <string>
#include <vector>
using namespace std;

class WeightData {
public:
	struct Entry {
		string		 	path;
		int 			weight;
	};

	vector<Entry*> 		List;

						WeightData();
						~WeightData();

	bool				Save(const char* path);
	bool				Load(const char* path);

	WeightData::Entry*	MatchEntry(string path);
	void				RemoveEntries(string path);

private:

};

#endif // _WEIGHT_DATA_H_
