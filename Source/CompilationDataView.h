/*
 * Copyright 2010-2017, BurnItNow Team. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _COMPILATIONDATAVIEW_H_
#define _COMPILATIONDATAVIEW_H_

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


class CompilationDataView : public BView {
public:
					CompilationDataView(BurnWindow& parent);
	virtual 		~CompilationDataView();

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

	BFilePanel* 	fOpenPanel;
	BTextView* 		fOutputView;
	BSeparatorView*	fInfoView;
	PathView*		fPathView;
	BTextControl* 	fDiscLabel;
	BButton*		fChooseButton;
	BButton*		fBuildButton;
	BButton*		fBurnButton;

	BPath* 			fDirPath;
	BPath* 			fImagePath;

	int64			fFolderSize;
	SizeView*		fSizeView;

	BNotification	fNotification;
	float			fProgress;
	BString			fETAtime;
	OutputParser	fParser;

	bool			fAbort;
	int				fAction;
};


#endif	// _COMPILATIONDATAVIEW_H_
