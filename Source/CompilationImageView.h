/*
 * Copyright 2010, BurnItNow Team. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _COMPILATIONIMAGEVIEW_H_
#define _COMPILATIONIMAGEVIEW_H_


#include <FilePanel.h>
#include <TextView.h>
#include <View.h>


class CommandThread;


class CompilationImageView : public BView {
public:
	CompilationImageView();
	virtual ~CompilationImageView();

	virtual void MessageReceived(BMessage* message);
	virtual void AttachedToWindow();

private:
	void _ChooseImage();
	void _OpenImage(BMessage* message);
	void _ImageScannerParse(BMessage* message);

	BFilePanel* fOpenPanel;
	BPath* fImagePath;
	CommandThread* fImageScannerThread;
	BTextView* fImageInfoTextView;
};


#endif	// _COMPILATIONIMAGEVIEW_H_
