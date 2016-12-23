/*
 * Copyright 2010-2016, BurnItNow Team. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _COMPILATIONCLONEVIEW_H_
#define _COMPILATIONCLONEVIEW_H_


#include "BurnWindow.h"

#include <FilePanel.h>
#include <Menu.h>
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

private:
	void 			_CreateImage();
	void 			_BurnImage();
	void 			_ClonerOutput(BMessage* message);

	BFilePanel*		fOpenPanel;
	CommandThread*	fClonerThread;
	BTextView*		fClonerInfoTextView;
	BurnWindow*		windowParent;
	BSeparatorView*	fClonerInfoBox;
	int				step;
};


#endif	// _COMPILATIONCLONEVIEW_H_
