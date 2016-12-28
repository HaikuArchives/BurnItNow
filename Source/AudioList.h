/*
 * Copyright 2015-2016. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *	Humdinger, humdingerb@gmail.com
 */

#ifndef AUDIOLIST_H
#define AUDIOLIST_H

#include <ListView.h>
#include <MenuItem.h>
#include <MessageRunner.h>
#include <PopUpMenu.h>

#include <Font.h>
#include <InterfaceDefs.h>
#include <ListItem.h>
#include <Path.h>
#include <String.h>

#include <Messenger.h>
#include <PopUpMenu.h>


class AudioListView : public BListView {
public:
					AudioListView(const char* name);
					~AudioListView();

	virtual void	AttachedToWindow();
	virtual void	Draw(BRect rect);
	virtual	void	FrameResized(float width, float height);
	virtual bool	InitiateDrag(BPoint point, int32 dragIndex,
						bool wasSelected);
	virtual	void	MessageReceived(BMessage* message);

	virtual	void	KeyDown(const char* bytes, int32 numBytes);
	void			MouseDown(BPoint position);
	void			MouseUp(BPoint position);
	virtual	void 	MouseMoved(BPoint where, uint32 transit,
						const BMessage* dragMessage);

	void			RenumberTracks();

private:
			void	GetSelectedItems(BList& indices);
			void	RemoveSelected(); // uses RemoveItemList()

	virtual	void	MoveItems(const BList& indices, int32 toIndex);
	virtual	void	RemoveItemList(const BList& indices);

	void			_ShowPopUpMenu(BPoint screen);

	bool			fShowingPopUpMenu;
	BRect			fDropRect;
};


class AudioListItem : public BListItem {
public:
					AudioListItem(BString filename, BString path, int32 track);
					~AudioListItem();

	virtual void	DrawItem(BView* view, BRect rect, bool complete = false);
	virtual	void	Update(BView* view, const BFont* finfo);

	BString			GetFilename() { return fFilename; };
	BString			GetPath() { return fPath; };
	void			SetTrack(int32 track) { fTrack = track; };

private:
	BString			fFilename;
	BString			fPath;
	int32			fTrack;
	bool			fUpdateNeeded;
};


class ContextPopUp : public BPopUpMenu {
public:
					ContextPopUp(const char* name, BMessenger target);
	virtual 		~ContextPopUp();

private:
	BMessenger 		fTarget;
};

#endif // AUDIOLIST_H
