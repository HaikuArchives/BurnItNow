/*
 * Copyright 2010-2017, BurnItNow Team. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _COMPILATIONCDRWVIEW_H_
#define _COMPILATIONCDRWVIEW_H_


#include "BurnWindow.h"
#include "OutputParser.h"

#include <FilePanel.h>
#include <Menu.h>
#include <Notification.h>
#include <SeparatorView.h>
#include <TextView.h>
#include <View.h>


class CommandThread;


class CompilationCDRWView : public BView {
public:
					CompilationCDRWView(BurnWindow& parent);
	virtual 		~CompilationCDRWView();

	virtual void	MessageReceived(BMessage* message);
	virtual void	AttachedToWindow();
	int32			InProgress();

private:
	void 			_Blank();
	void 			_BlankerParserOutput(BMessage* message);
	void			_UpdateProgress();

	BFilePanel*		fOpenPanel;
	CommandThread*	fBlankerThread;
	BTextView*		fBlankerInfoTextView;
	BurnWindow*		windowParent;
	BSeparatorView*	fBlankerInfoBox;
	BMenu*			fBlankModeMenu;

	BNotification	fNotification;
	float			fProgress;
	BString			fETAtime;
	OutputParser	fParser;

	int32			step;
};


#endif	// _COMPILATIONCDRWVIEW_H_
