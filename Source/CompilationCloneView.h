/*
 * Copyright 2010-2017, BurnItNow Team. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _COMPILATIONCLONEVIEW_H_
#define _COMPILATIONCLONEVIEW_H_

#include <Button.h>
#include <FilePanel.h>
#include <Menu.h>
#include <SeparatorView.h>
#include <TextView.h>
#include <View.h>

#include "BurnWindow.h"
#include "OutputParser.h"
#include "SizeView.h"


class CommandThread;


class CompilationCloneView : public BView {
public:
					CompilationCloneView(BurnWindow& parent);
	virtual 		~CompilationCloneView();

	virtual void	AttachedToWindow();
	virtual void	MessageReceived(BMessage* message);

	int32			InProgress();

private:
	void 			_Build();
	void 			_BuildOutput(BMessage* message);
	void 			_Burn();
	void 			_BurnOutput(BMessage* message);
	void 			_GetImageInfo();
	void 			_GetImageInfoOutput(BMessage* message);
	void			_UpdateProgress(const char* title);
	void			_UpdateSizeBar();

	CommandThread*	fBurnerThread;
	BurnWindow*		fWindowParent;

	BFilePanel*		fOpenPanel;
	BTextView*		fOutputView;
	BSeparatorView*	fInfoView;
	BButton*		fBurnButton;
	BButton*		fBuildButton;

	int64			fImageSize;
	SizeView*		fSizeView;

	BString			fNoteID;
	int32			fID;
	float			fProgress;
	BString			fETAtime;
	OutputParser	fParser;

	int32			fAbort;
	int32			fAction;
	bool			fAudioMode;
};


#endif	// _COMPILATIONCLONEVIEW_H_
