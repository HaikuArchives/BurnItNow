/*
 * Copyright 2010-2017, BurnItNow Team. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _COMPILATIONCDRWVIEW_H_
#define _COMPILATIONCDRWVIEW_H_

#include <Button.h>
#include <FilePanel.h>
#include <Menu.h>
#include <SeparatorView.h>
#include <TextView.h>
#include <View.h>

#include "BurnWindow.h"



class CommandThread;


class CompilationBlankView : public BView {
public:
					CompilationBlankView(BurnWindow& parent);
	virtual 		~CompilationBlankView();

	virtual void	AttachedToWindow();
	virtual void	MessageReceived(BMessage* message);

	int32			InProgress();

private:
	void 			_Blank();
	void 			_BlankOutput(BMessage* message);

	CommandThread*	fBlankerThread;
	BurnWindow*		fWindowParent;

	BTextView*		fOutputView;
	BSeparatorView*	fInfoView;
	BMenu*			fBlankModeMenu;
	BButton*		fBlankButton;

	BString			fNoteID;
	int32			fID;
	int32			fAction;
};


#endif	// _COMPILATIONCDRWVIEW_H_
