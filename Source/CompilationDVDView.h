/*
 * Copyright 2010-2017, BurnItNow Team. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _COMPILATIONDVDVIEW_H_
#define _COMPILATIONDVDVIEW_H_


#include "BurnWindow.h"
#include "PathView.h"
#include "SizeView.h"

#include <Button.h>
#include <FilePanel.h>
#include <Menu.h>
#include <SeparatorView.h>
#include <TextView.h>
#include <View.h>


class CommandThread;


class DVDRefFilter : public BRefFilter {
public:
					DVDRefFilter() {};
	virtual			~DVDRefFilter() {};

	bool			Filter(const entry_ref* ref, BNode* node,
						struct stat_beos* st, const char* filetype);
};


class CompilationDVDView : public BView {
public:
					CompilationDVDView(BurnWindow& parent);
	virtual 		~CompilationDVDView();

	virtual void	MessageReceived(BMessage* message);
	virtual void	AttachedToWindow();
	
	void			BuildISO();
	void			BurnDisc();

private:
	void 			_ChooseDirectory();
	void 			_FromScratch();
	void 			_OpenDirectory(BMessage* message);
	void			_GetFolderSize();
	void 			_BurnerOutput(BMessage* message);
	void			_UpdateSizeBar();

	BFilePanel* 	fOpenPanel;
	CommandThread* 	fBurnerThread;
	BTextView* 		fBurnerInfoTextView;
	BurnWindow* 	windowParent;
	BSeparatorView*	fBurnerInfoBox;
	PathView*		fPathView;
	BButton*		fDVDButton;
	BButton*		fImageButton;
	BButton*		fBurnButton;

	BPath* 			fDirPath;
	BPath* 			fImagePath;
	int				step;

	int64			fFolderSize;
	SizeView*		fSizeView;
};


#endif	// _COMPILATIONDVDVIEW_H_
