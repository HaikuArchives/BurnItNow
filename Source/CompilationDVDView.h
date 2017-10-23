/*
 * Copyright 2010-2017, BurnItNow Team. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _COMPILATIONDVDVIEW_H_
#define _COMPILATIONDVDVIEW_H_

#include <Button.h>
#include <FilePanel.h>
#include <Menu.h>
#include <Notification.h>
#include <SeparatorView.h>
#include <TextControl.h>
#include <TextView.h>
#include <View.h>

#include "BurnWindow.h"
#include "CompilationShared.h"
#include "OutputParser.h"
#include "SizeView.h"


class CommandThread;


class CompilationDVDView : public BView {
public:
					CompilationDVDView(BurnWindow& parent);
	virtual 		~CompilationDVDView();

	virtual void	AttachedToWindow();
	virtual void	MessageReceived(BMessage* message);
	
	int32			InProgress();

private:
	void			_Build();
	void 			_BuildOutput(BMessage* message);
	void			_Burn();
	void 			_BurnOutput(BMessage* message);
	void 			_ChooseDirectory();
	void			_GetFolderSize();
	void 			_OpenDirectory(BMessage* message);
	void			_UpdateProgress();
	void			_UpdateSizeBar();

	CommandThread* 	fBurnerThread;
	BurnWindow* 	fWindowParent;
	BTextView* 		fOutputView;
	BFilePanel* 	fOpenPanel;
	BSeparatorView*	fInfoView;
	PathView*		fPathView;
	BTextControl* 	fDiscLabel;
	BButton*		fDVDButton;
	BButton*		fBuildButton;
	BButton*		fBurnButton;

	BPath* 			fDirPath;
	BPath* 			fImagePath;
	const char*		fDVDMode;

	int64			fFolderSize;
	SizeView*		fSizeView;

	BNotification	fNotification;
	float			fProgress;
	BString			fETAtime;
	OutputParser	fParser;

	bool			fAbort;
	int				fAction;
};


#endif	// _COMPILATIONDVDVIEW_H_
