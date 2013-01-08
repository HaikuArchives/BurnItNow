/*
 * Copyright 2010-2012, BurnItNow Team. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _COMPILATIONAUDIOVIEW_H_
#define _COMPILATIONAUDIOVIEW_H_


#include "BurnWindow.h"

#include <Box.h>
#include <ListView.h>
#include <TextView.h>
#include <View.h>


class CommandThread;


class CompilationAudioView : public BView {
public:
	CompilationAudioView(BurnWindow &parent);
	virtual ~CompilationAudioView();

	virtual void MessageReceived(BMessage* message);

private:
	void _BurnerParserOutput(BMessage* message);

	CommandThread* fBurnerThread;
	BTextView* fBurnerInfoTextView;
	BListView* fAudioList;
	BurnWindow* windowParent;
	BBox* fBurnerInfoBox;
	BBox* fAudioBox;
};


#endif	// _COMPILATIONAUDIOVIEW_H_
