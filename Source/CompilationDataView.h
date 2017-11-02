/*
 * Copyright 2010-2017, BurnItNow Team. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _COMPILATIONDATAVIEW_H_
#define _COMPILATIONDATAVIEW_H_

#include <Button.h>
#include <FilePanel.h>
#include <Menu.h>
#include <MessageRunner.h>
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
	void			_UpdateProgress(const char* title);
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

	BString			fNoteID;
	int32			fID;
	float			fProgress;
	BString			fETAtime;
	OutputParser	fParser;

	int32			fAbort;
	int32			fAction;
	BMessageRunner*	fRunner;
};


#endif	// _COMPILATIONDATAVIEW_H_
