/*
 * Copyright 2015 Your Name <your@email.address>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
 
#ifndef DIRREFFILTER_H
#define DIRREFFILTER_H


#include <FilePanel.h>


class DirRefFilter : public BRefFilter {
public:
					DirRefFilter() {};
	virtual			~DirRefFilter() {};

	bool			Filter(const entry_ref* ref, BNode* node,
						struct stat_beos* st, const char* filetype);
};


#endif // DIRREFFILTER_H
