/*
 * Copyright 2010-2017, BurnItNow Team. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _COMPILATIONDVDVIEW_H_
#define _COMPILATIONDVDVIEW_H_


#include "BurnWindow.h"
#include "OutputParser.h"
#include "PathView.h"
#include "SizeView.h"

#include <Button.h>
#include <FilePanel.h>
#include <Menu.h>
#include <Notification.h>
#include <SeparatorView.h>
#include <TextControl.h>
#include <TextView.h>
#include <View.h>


class CommandThread;


class CompilationDVDView : public BView {
public:
					CompilationDVDView(BurnWindow& parent);
	virtual 		~CompilationDVDView();

	virtual void	MessageReceived(BMessage* message);
	virtual void	AttachedToWindow();
	
	void			BuildISO();
	void			BurnDisc();
	int32			InProgress();

private:
	void 			_ChooseDirectory();
	void 			_FromScratch();
	void 			_OpenDirectory(BMessage* message);
	void			_GetFolderSize();
	void 			_BurnerOutput(BMessage* message);
	void			_UpdateProgress();
	void			_UpdateSizeBar();

	BFilePanel* 	fOpenPanel;
	CommandThread* 	fBurnerThread;
	BTextView* 		fBurnerInfoTextView;
	BurnWindow* 	windowParent;
	BSeparatorView*	fBurnerInfoBox;
	PathView*		fPathView;
	BTextControl* 	fDiscLabel;
	BButton*		fDVDButton;
	BButton*		fImageButton;
	BButton*		fBurnButton;

	BPath* 			fDirPath;
	BPath* 			fImagePath;
	const char*		fDVDMode;
	int				step;

	int64			fFolderSize;
	SizeView*		fSizeView;

	BNotification	fNotification;
	float			fProgress;
	BString			fETAtime;
	OutputParser	fParser;
};


#endif	// _COMPILATIONDVDVIEW_H_
