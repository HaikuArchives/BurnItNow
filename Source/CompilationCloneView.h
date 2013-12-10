/*
 * Copyright 2010-2012, BurnItNow Team. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _COMPILATIONCLONEVIEW_H_
#define _COMPILATIONCLONEVIEW_H_


#include "BurnWindow.h"

#include <Box.h>
#include <FilePanel.h>
#include <Menu.h>
#include <TextView.h>
#include <View.h>


class CommandThread;


class CompilationCloneView : public BView {
public:
	CompilationCloneView(BurnWindow &parent);
	virtual ~CompilationCloneView();

	virtual void MessageReceived(BMessage* message);
	virtual void AttachedToWindow();

private:
	void _CreateImage();
	void _BurnImage();
	void _ClonerOutput(BMessage* message);

	BFilePanel* fOpenPanel;
	CommandThread* fClonerThread;
	BTextView* fClonerInfoTextView;
	BurnWindow* windowParent;
	BBox* fClonerInfoBox;
	BMenu* fSourceDeviceMenu;
};


#endif	// _COMPILATIONCLONEVIEW_H_
