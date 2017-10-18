/*
 * Copyright 2010-2017, BurnItNow Team. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _COMPILATIONCLONEVIEW_H_
#define _COMPILATIONCLONEVIEW_H_


#include "BurnWindow.h"
#include "OutputParser.h"
#include "SizeView.h"

#include <Button.h>
#include <FilePanel.h>
#include <Menu.h>
#include <Notification.h>
#include <SeparatorView.h>
#include <TextView.h>
#include <View.h>


class CommandThread;


class CompilationCloneView : public BView {
public:
					CompilationCloneView(BurnWindow& parent);
	virtual 		~CompilationCloneView();

	virtual void	MessageReceived(BMessage* message);
	virtual void	AttachedToWindow();
	int32			InProgress();

private:
	void 			_CreateImage();
	void 			_BurnImage();
	void 			_ClonerOutput(BMessage* message);
	void			_UpdateProgress();
	void			_UpdateSizeBar();

	BFilePanel*		fOpenPanel;
	CommandThread*	fClonerThread;
	BTextView*		fClonerInfoTextView;
	BurnWindow*		windowParent;
	BSeparatorView*	fClonerInfoBox;
	BButton*		fBurnButton;
	BButton*		fImageButton;

	int				step;
	SizeView*		fSizeView;

	BNotification	fNotification;
	float			fProgress;
	BString			fETAtime;
	OutputParser	fParser;
};


#endif	// _COMPILATIONCLONEVIEW_H_
