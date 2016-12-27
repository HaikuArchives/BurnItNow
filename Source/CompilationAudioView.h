/*
 * Copyright 2010-2016, BurnItNow Team. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _COMPILATIONAUDIOVIEW_H_
#define _COMPILATIONAUDIOVIEW_H_


#include "AudioList.h"
#include "BurnWindow.h"

#include <Button.h>
#include <ListView.h>
#include <Path.h>
#include <SeparatorView.h>
#include <TextView.h>
#include <View.h>


#define MAX_TRACKS 255


class CommandThread;


class CompilationAudioView : public BView {
public:
					CompilationAudioView(BurnWindow& parent);
	virtual 		~CompilationAudioView();

	virtual void	AttachedToWindow();
	virtual void	MessageReceived(BMessage* message);
	
	void 			BurnDisc();

private:
	void 			_BurnerParserOutput(BMessage* message);
	void 			_AddTrack(BMessage* message);
	
	CommandThread*	fBurnerThread;
	BTextView* 		fBurnerInfoTextView;
	AudioListView* 	fTrackList;
	BurnWindow* 	windowParent;
	BSeparatorView*	fBurnerInfoBox;
	BSeparatorView*	fAudioBox;
	BButton*		fBurnButton;
	
	BPath* 			fTrackPaths[MAX_TRACKS];
	int 			fCurrentPath;
};


#endif	// _COMPILATIONAUDIOVIEW_H_
