/*
 * Copyright 2017, BurnItNow Team. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef COMPILATIONSHARED_H
#define COMPILATIONSHARED_H

#include <FilePanel.h>
#include <SupportDefs.h>
#include <StringView.h>


class DirRefFilter : public BRefFilter {
public:
					DirRefFilter() {};
	virtual			~DirRefFilter() {};

	bool			Filter(const entry_ref* ref, BNode* node,
						struct stat_beos* st, const char* filetype);
};


class BurnWindow;

class PathView : public BStringView {
public:
			PathView(const char* name, const char* text);
			~PathView();

	void	MouseDown(BPoint position);
};


bool CheckFreeSpace(int64 size, const char* cache);
int32 FolderSizeCount(void* arg);
BString	GetExtension(const entry_ref* ref);

#endif // COMPILATIONSHARED_H
