/*
 * Copyright 2010-2017, BurnItNow Team. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _COMPILATIONIMAGEVIEW_H_
#define _COMPILATIONIMAGEVIEW_H_

#include <Button.h>
#include <FilePanel.h>
#include <Notification.h>
#include <SeparatorView.h>
#include <TextView.h>
#include <View.h>

#include "BurnWindow.h"
#include "OutputParser.h"
#include "PathView.h"
#include "SizeView.h"


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

	virtual void 	AttachedToWindow();
	virtual void	MessageReceived(BMessage* message);

	int32			InProgress();

private:
	void 			_BurnImage();
	void 			_BurnOutput(BMessage* message);
	void 			_ChooseImage();
	void 			_OpenImage(BMessage* message);
	void			_UpdateProgress();
	void			_UpdateSizeBar();

	CommandThread* 	fImageParserThread;
	BurnWindow*		fWindowParent;

	BFilePanel* 	fOpenPanel;
	BTextView*		fOutputView;
	BSeparatorView*	fInfoView;
	PathView*		fPathView;
	BPath* 			fImagePath;
	BButton*		fChooseButton;
	BButton*		fBurnButton;

	SizeView*		fSizeView;

	BNotification	fNotification;
	float			fProgress;
	BString			fETAtime;
	OutputParser	fParser;

	int				fAction;
};


#endif	// _COMPILATIONIMAGEVIEW_H_
