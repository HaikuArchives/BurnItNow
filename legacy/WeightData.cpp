/*
 * Copyright 2012 Tri-Edge AI <triedgeai@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
 
#include "WeightData.h"

#include <iostream>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>
using namespace std;

WeightData::WeightData()
{
	
}


WeightData::~WeightData()
{
	for (unsigned int i = 0; i < List.size(); i++)
		delete List[i];	
		
	List.clear();
}


bool
WeightData::Save(const char* path)
{
	fstream file(path, ios::out);
	
	if (!file)
		return false;

	for (unsigned int i = 0; i < List.size(); i++)
		if (List[i]->weight != 0)
			file<<List[i]->path<<"\t"<<List[i]->weight<<endl;
		
	file.close();
	return true;
}


bool
WeightData::Load(const char* path)
{	
	fstream file(path, ios::in);
	string line;
		
	if (!file)
		return false;
		
	while (!file.eof()) {
		WeightData::Entry* ent = new Entry;
		
		getline(file, line);
		
		if (line.length() < 2)
			continue;
		
		int pos = line.rfind('\t');
		ent->path = line.substr(0, pos);
		ent->weight = atoi(line.substr(pos + 1).c_str());

		List.push_back(ent);
	}

	file.close();
	return true;
}


WeightData::Entry*
WeightData::MatchEntry(string path)
{
	for (unsigned int i = 0; i < List.size(); i++) {
		if (List[i]->path == path) {
			WeightData::Entry* ent = List[i];
			return ent;	
		}
	}
	
	return NULL;
}


void
WeightData::RemoveEntries(string path)
{	
	for (unsigned int i = 0; i < List.size(); i++) {
		if (List[i]->path.find(path + '/') == 0
			|| List[i]->path == path) {
			
			delete List[i];
			List.erase(List.begin() + i);
			i--;
		}
	}	
}
