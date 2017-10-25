/*
 * Copyright 2017, BurnItNow Team. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#include <compat/sys/stat.h>
#include <stdio.h>
#include <string>

#include <Alert.h>
#include <Catalog.h>
#include <Entry.h>
#include <Messenger.h>
#include <Node.h>
#include <Path.h>
#include <String.h>
#include <StringForSize.h>
#include <Volume.h>

#include "CompilationShared.h"
#include "Constants.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Helpers"


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
	    std::string cmd("du -sb \"");
	    cmd.append(path);
	    cmd.append("\" | cut -f1 2>&1");
	
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


bool
CheckFreeSpace(int64 size, const char* cache)
{
	dev_t device = dev_for_path(cache);
	BVolume volume(device);
	if (volume.InitCheck() != B_OK)
		return false;

	off_t spaceLeft = volume.FreeBytes();	
	if (spaceLeft < size) {
		char amount[B_PATH_NAME_LENGTH];
		string_for_size(size - spaceLeft, amount, sizeof(amount));
		BString text(B_TRANSLATE(
			"There's not enough free space available at '%cache%'. "
			"We're %amount% short.\n\n"
			"Make room, or change the cache folder."));
		text.ReplaceFirst("%cache%", cache);
		text.ReplaceFirst("%amount%", amount);
		(new BAlert("FreeSpaceAlert", text, B_TRANSLATE("OK")))->Go();

		return false;
	}
	return true;
}


PathView::PathView(const char* name, const char* text)
	:
	BStringView(name, text)
{
	SetFontSize(be_plain_font->Size() - 2);
	SetHighColor(tint_color(ui_color(B_CONTROL_TEXT_COLOR), 0.7));
	SetAlignment(B_ALIGN_RIGHT);
}


PathView::~PathView()
{
}


void
PathView::MouseDown(BPoint position)
{
	BString contents(Text());

	if (contents.FindFirst("/") == B_ERROR) {
		BMessage message(kChooseButton);
		BMessenger msgr(Parent());
		msgr.SendMessage(&message);
	} else {	
		BEntry entry(contents);
		BNode node(&entry);
		if (node.InitCheck() != B_OK)
			return;

		entry_ref folderRef;
		entry.GetRef(&folderRef);

		if (!node.IsDirectory()) {
			BEntry parent;
			entry.GetParent(&parent);
			parent.GetRef(&folderRef);
		}
	
		BMessenger msgr("application/x-vnd.Be-TRAK");
		BMessage refMsg(B_REFS_RECEIVED);
		refMsg.AddRef("refs",&folderRef);
		msgr.SendMessage(&refMsg);
	}

	BStringView::MouseDown(position);
}
