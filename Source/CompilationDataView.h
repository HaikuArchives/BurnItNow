/*
 * Copyright 2010-2017, BurnItNow Team. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _COMPILATIONDATAVIEW_H_
#define _COMPILATIONDATAVIEW_H_


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


class CompilationDataView : public BView {
public:
					CompilationDataView(BurnWindow& parent);
	virtual 		~CompilationDataView();

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
	BButton*		fChooseButton;
	BButton*		fImageButton;
	BButton*		fBurnButton;

	BPath* 			fDirPath;
	BPath* 			fImagePath;
	int				step;

	int64			fFolderSize;
	SizeView*		fSizeView;

	BNotification	fNotification;
	float			fProgress;
	BString			fETAtime;
	OutputParser	fParser;
};


#endif	// _COMPILATIONDATAVIEW_H_
