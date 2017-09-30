/*
 * Copyright 2017, BurnItNow Team. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#include "FolderSizeCount.h"
#include "Constants.h"

#include <Messenger.h>
#include <Path.h>
#include <String.h>

#include <stdio.h>
#include <string>


int32
FolderSizeCount(void* arg)
{
	BMessage* msg = static_cast<BMessage *>(arg);

	BString path;
	BMessenger from;
	msg->FindString("path", &path);
	msg->FindMessenger("from", &from);

	off_t folderSize = 0;
	BPath folder(path);
	if (folder.InitCheck() == B_OK) {
	    // command to be executed
	    std::string cmd("du -sb ");
	    cmd.append(path);
	    cmd.append(" | cut -f1 2>&1");
	
	    // execute above command and get the output
	    FILE *stream = popen(cmd.c_str(), "r");
	    if (stream) {
	        const int max_size = 256;
	        char readbuf[max_size];
	        if (fgets(readbuf, max_size, stream) != NULL)
	            folderSize = atoll(readbuf);
	        pclose(stream);            
	    }
	}
	msg = new BMessage(kSetFolderSize);
	msg->AddInt64("foldersize", folderSize / 1024); // size in KiB
	from.SendMessage(msg);
	
	return 0;
}
