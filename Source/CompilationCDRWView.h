/*
 * Copyright 2010-2017, BurnItNow Team. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _COMPILATIONCDRWVIEW_H_
#define _COMPILATIONCDRWVIEW_H_


#include <FilePanel.h>
#include <Menu.h>
#include <Notification.h>
#include <SeparatorView.h>
#include <TextView.h>
#include <View.h>

#include "BurnWindow.h"
#include "OutputParser.h"



class CommandThread;


class CompilationCDRWView : public BView {
public:
					CompilationCDRWView(BurnWindow& parent);
	virtual 		~CompilationCDRWView();

	virtual void	AttachedToWindow();
	virtual void	MessageReceived(BMessage* message);

	int32			InProgress();

private:
	void 			_Blank();
	void 			_BlankOutput(BMessage* message);
	void			_UpdateProgress();

	CommandThread*	fBlankerThread;
	BurnWindow*		fWindowParent;

	BFilePanel*		fOpenPanel;
	BTextView*		fOutputView;
	BSeparatorView*	fInfoView;
	BMenu*			fBlankModeMenu;

	BNotification	fNotification;
	float			fProgress;
	BString			fETAtime;
	OutputParser	fParser;

	int32			fAction;
};


#endif	// _COMPILATIONCDRWVIEW_H_
