/*
 * Copyright 2010-2012, BurnItNow Team. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _COMPILATIONIMAGEVIEW_H_
#define _COMPILATIONIMAGEVIEW_H_


#include "BurnWindow.h"

#include <Box.h>
#include <FilePanel.h>
#include <TextView.h>
#include <View.h>


class CommandThread;


class CompilationImageView : public BView {
public:
	CompilationImageView(BurnWindow &parent);
	virtual ~CompilationImageView();

	virtual void MessageReceived(BMessage* message);
	virtual void AttachedToWindow();

private:
	void _ChooseImage();
	void _BurnImage();
	void _OpenImage(BMessage* message);
	void _ImageParserOutput(BMessage* message);

	BFilePanel* fOpenPanel;
	BPath* fImagePath;
	CommandThread* fImageParserThread;
	BTextView* fImageInfoTextView;
	BurnWindow* windowParent;
	BBox* fImageInfoBox;
};


#endif	// _COMPILATIONIMAGEVIEW_H_
