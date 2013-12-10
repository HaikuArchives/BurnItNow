/*
 * Copyright 2010-2012, BurnItNow Team. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _COMPILATIONDATAVIEW_H_
#define _COMPILATIONDATAVIEW_H_


#include "BurnWindow.h"

#include <Box.h>
#include <FilePanel.h>
#include <Menu.h>
#include <TextView.h>
#include <View.h>


class CommandThread;


class CompilationDataView : public BView {
public:
	CompilationDataView(BurnWindow &parent);
	virtual ~CompilationDataView();

	virtual void MessageReceived(BMessage* message);
	virtual void AttachedToWindow();
	
	void BuildISO();
	void BurnDisc();

private:
	void _ChooseDirectory();
	void _FromScratch();
	void _OpenDirectory(BMessage* message);
	void _BurnerOutput(BMessage* message);

	BFilePanel* fOpenPanel;
	CommandThread* fBurnerThread;
	BTextView* fBurnerInfoTextView;
	BurnWindow* windowParent;
	BBox* fBurnerInfoBox;
	BPath* fDirPath;
};


#endif	// _COMPILATIONDATAVIEW_H_
