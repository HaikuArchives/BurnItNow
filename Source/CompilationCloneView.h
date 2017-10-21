/*
 * Copyright 2010-2017, BurnItNow Team. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _COMPILATIONCLONEVIEW_H_
#define _COMPILATIONCLONEVIEW_H_

#include <Button.h>
#include <FilePanel.h>
#include <Menu.h>
#include <Notification.h>
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
	void			_UpdateProgress();
	void			_UpdateSizeBar();

	CommandThread*	fClonerThread;
	BurnWindow*		fWindowParent;

	BFilePanel*		fOpenPanel;
	BTextView*		fOutputView;
	BSeparatorView*	fInfoView;
	BButton*		fBurnButton;
	BButton*		fBuildButton;

	SizeView*		fSizeView;

	BNotification	fNotification;
	float			fProgress;
	BString			fETAtime;
	OutputParser	fParser;

	int				fAction;
};


#endif	// _COMPILATIONCLONEVIEW_H_
