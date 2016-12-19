/*
 * Copyright 2010-2012, BurnItNow Team. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _COMPILATIONCDRWVIEW_H_
#define _COMPILATIONCDRWVIEW_H_


#include "BurnWindow.h"

#include <Box.h>
#include <FilePanel.h>
#include <Menu.h>
#include <SeparatorView.h>
#include <TextView.h>
#include <View.h>


class CommandThread;


class CompilationCDRWView : public BView {
public:
	CompilationCDRWView(BurnWindow &parent);
	virtual ~CompilationCDRWView();

	virtual void MessageReceived(BMessage* message);
	virtual void AttachedToWindow();

private:
	void _Blank();
	void _BlankerParserOutput(BMessage* message);

	BFilePanel* fOpenPanel;
	CommandThread* fBlankerThread;
	BTextView* fBlankerInfoTextView;
	BurnWindow* windowParent;
	BSeparatorView* fBlankerInfoBox;
	BMenu* fBlankModeMenu;
};


#endif	// _COMPILATIONCDRWVIEW_H_
