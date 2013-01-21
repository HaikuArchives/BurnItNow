/*
 * Copyright 2000-2002, Johan Nilsson. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _LEFTLIST_H_
#define _LEFTLIST_H_


#include <Entry.h>
#include <ListView.h>
#include <PopUpMenu.h>


struct AudioInfo {
	int32 bps;
	int32 frame_rate;
	int32 channels;
	char pretty_name[200];
	char short_name[100];
	bigtime_t total_time;
};


class LeftListItem : public BListItem {
public:
	LeftListItem(entry_ref* ref, const char* name, BBitmap* icon, struct AudioInfo* Info);
	virtual void DrawItem(BView* owner, BRect frame, bool complete);

	BBitmap* fIconBitmap;
	char fName[1024];
	entry_ref fRef;
	struct AudioInfo fAudioInfo;
	bool fIsAudio;
};


class LeftList : public BListView {
public:
	LeftList(BRect size);
	~LeftList();
	virtual void KeyDown(const char* bytes, int32 numBytes);
	virtual void MouseDown(BPoint point);
	virtual void WriteLog(const char* string);
	virtual void MessageReceived(BMessage* message);

	uint32 fLastButton, fClickCount;
	BBitmap* fISOBitmap;
	BBitmap* fVRCDBitmap;
	BBitmap* fAudioBitmap;
	BPopUpMenu* fTrackPopUpMenu;
};


#endif	// _LEFTLIST_H_
