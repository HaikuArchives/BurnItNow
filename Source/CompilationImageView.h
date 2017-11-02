/*
 * Copyright 2010-2017, BurnItNow Team. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _COMPILATIONIMAGEVIEW_H_
#define _COMPILATIONIMAGEVIEW_H_

#include <Button.h>
#include <FilePanel.h>
#include <SeparatorView.h>
#include <TextView.h>
#include <View.h>

#include "BurnWindow.h"
#include "CompilationShared.h"
#include "OutputParser.h"
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
	void 			_Burn();
	void 			_BurnOutput(BMessage* message);
	void 			_ChooseImage();
	void 			_OpenImage(BMessage* message);
	void 			_OpenOutput(BMessage* message);
	void			_UpdateProgress(const char* title);
	void			_UpdateSizeBar();

	CommandThread* 	fBurnerThread;
	BurnWindow*		fWindowParent;

	BFilePanel* 	fOpenPanel;
	BTextView*		fOutputView;
	BSeparatorView*	fInfoView;
	PathView*		fPathView;
	BPath* 			fImagePath;
	BButton*		fChooseButton;
	BButton*		fBurnButton;

	SizeView*		fSizeView;

	BString			fNoteID;
	int32			fID;
	float			fProgress;
	BString			fETAtime;
	OutputParser	fParser;

	int32			fAbort;
	int32			fAction;
};


#endif	// _COMPILATIONIMAGEVIEW_H_
