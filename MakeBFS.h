/*
 * Copyright 2000-2002, Johan Nilsson. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _MAKEBFS_H_
#define _MAKEBFS_H_


#include <Entry.h>


#define BUFFERLEN 1024 * 512


status_t CopyFile(const entry_ref& source, const entry_ref& dest);

status_t CopyFiles();

status_t MountBFS();

status_t UnmountBFS();

status_t MakeBFS();

uint64 GetBFSSize();


#endif	// _MAKEBFS_H_
