/*
 * Copyright 2010-2017, BurnItNow Team. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _COMPILATIONIMAGEVIEW_H_
#define _COMPILATIONIMAGEVIEW_H_


#include "BurnWindow.h"
#include "OutputParser.h"
#include "PathView.h"
#include "SizeView.h"

#include <Button.h>
#include <FilePanel.h>
#include <Notification.h>
#include <SeparatorView.h>
#include <TextView.h>
#include <View.h>


class CommandThread;


class ImageRefFilter : public BRefFilter {
public:
					ImageRefFilter() {};
	virtual			~ImageRefFilter() {};

	bool			Filter(const entry_ref* ref, BNode* node,
						struct stat_beos* st, const char* filetype);
};


class CompilationImageView : public BView {
public:
					CompilationImageView(BurnWindow& parent);
	virtual 		~CompilationImageView();

	virtual void	MessageReceived(BMessage* message);
	virtual void 	AttachedToWindow();
	int32			InProgress();

private:
	void 			_ChooseImage();
	void 			_BurnImage();
	void 			_OpenImage(BMessage* message);
	void 			_ImageParserOutput(BMessage* message);
	void 			_ImageBurnerOutput(BMessage* message);
	void			_UpdateProgress();
	void			_UpdateSizeBar();

	BFilePanel* 	fOpenPanel;
	BPath* 			fImagePath;
	CommandThread* 	fImageParserThread;
	BTextView*		fImageInfoTextView;
	BurnWindow*		windowParent;
	BSeparatorView*	fImageInfoBox;
	PathView*		fPathView;
	BButton*		fChooseButton;
	BButton*		fBurnButton;

	int				step;
	SizeView*		fSizeView;

	BNotification	fNotification;
	float			fProgress;
	BString			fETAtime;
	OutputParser	fParser;
};


#endif	// _COMPILATIONIMAGEVIEW_H_
