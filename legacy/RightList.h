/*
 * Copyright 2000-2002, Johan Nilsson. All rights reserved.
 * Copyright 2011 BurnItNow maintainers
 * Distributed under the terms of the MIT License.
 */
 
#ifndef _RIGHTLIST_H_
#define _RIGHTLIST_H_

#include "WeightData.h"

#include <Entry.h>
#include <FilePanel.h>
#include <ListView.h>
#include <PopUpMenu.h>

class FileListItem : public BListItem {
public:
						FileListItem(entry_ref* ref, uint32 ix, char* name, 
							BBitmap* icon);
						~FileListItem();
	
	virtual void 		DrawItem(BView* owner, BRect frame, bool complete);

	BBitmap* 			fIconBitmap;
	char* 				fName;
	entry_ref 			fRef;
	uint32 				fIndex;
	WeightData::Entry*	fWeight;
};

class RightList : public BListView {
public:
	RightList(BRect size);
	~RightList();
	
	virtual void 		Draw(BRect updateRect);
	virtual void 		UpdateInfo(char* str1, char* str2, 
							char* str3, char* str4, char* str5);
	virtual void 		UpdateDir();
	virtual void		UpdateWeights();
	virtual void		UpdateIndexes();
	virtual void 		KeyDown(const char* bytes, int32 numBytes);
	virtual void 		MouseDown(BPoint point);
	virtual void		SelectionChanged();
	virtual void 		ParentDir();
	virtual void 		CreateDir();
	virtual void 		MakeLink(entry_ref* ref);
	virtual void 		MakeDir(entry_ref* ref);
	virtual void 		MakeDir(const char* name);
	virtual void 		WriteLog(const char* string);
	virtual void 		MessageReceived(BMessage* message);
	virtual void 		AttachedToWindow();
	virtual void 		DeleteFromVRCD(entry_ref* ref);
	virtual void 		DeleteDirFromVRCD(entry_ref* ref);

			void		Add();
			void		AddDone();
			void 		RemoveSelected();
			void		MoveSelectedUp();
			void		MoveSelectedDown();

	string				fPath;
	BDirectory* 		fBDirectory;
	BDirectory*			fTDirectory;
	
	uint32 				fLastButton;
	uint32				fClickCount;
	
	BBitmap* 			fFileBitmap;
	BBitmap* 			fDirectoryBitmap;
	BBitmap* 			fInfoBitmap;
	BBitmap* 			fAudioBitmap;
	
	BPopUpMenu* 		fItemPopUpMenu;
	BMenuItem*			fItemMakeDir;
	BMenuItem*			fItemAdd;
	BMenuItem*			fItemRemove;
	BMenuItem*			fItemUp;
	BMenuItem*			fItemDown;
	
	BFilePanel*			fPanelAdd;
	
	WeightData			fWeights;

private:
	static 	int			_SortByWeightCallback(const void* a, const void* b);
};

#endif	// _RIGHTLIST_H_
