/*
 * Copyright 2015 Haiku Inc.
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "DirRefFilter.h"

#include <compat/sys/stat.h>

bool
DirRefFilter::Filter(const entry_ref* ref, BNode* node,
	struct stat_beos* stat, const char* filetype)
{
	if (S_ISDIR(stat->st_mode))
		return true;

	if (S_ISLNK(stat->st_mode)) {
		// Traverse symlinks
		BEntry entry(ref, true);
		return entry.IsDirectory();
	}

	return false;
}
