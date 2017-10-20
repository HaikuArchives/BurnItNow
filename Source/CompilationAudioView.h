/*
 * Copyright 2010-2017, BurnItNow Team. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _COMPILATIONAUDIOVIEW_H_
#define _COMPILATIONAUDIOVIEW_H_

#include <Button.h>
#include <ListView.h>
#include <Notification.h>
#include <Path.h>
#include <SeparatorView.h>
#include <SplitView.h>
#include <TextView.h>
#include <View.h>

#include "AudioList.h"
#include "BurnWindow.h"
#include "OutputParser.h"
#include "SizeView.h"

#define MAX_TRACKS 255


class CommandThread;


class CompilationAudioView : public BView {
public:
					CompilationAudioView(BurnWindow& parent);
	virtual 		~CompilationAudioView();

	virtual void	AttachedToWindow();
	virtual void	MessageReceived(BMessage* message);
	
	int32			InProgress();
	BSplitView*		fAudioSplitView;

private:
	void 			_AddTrack(BMessage* message);
	void 			_Burn();
	void 			_BurnOutput(BMessage* message);
	void			_UpdateProgress();
	void			_UpdateSizeBar();
	
	CommandThread*	fBurnerThread;
	BurnWindow* 	fWindowParent;

	BTextView* 		fOutputView;
	BSeparatorView*	fInfoView;
	BSeparatorView*	fAudioBox;
	BButton*		fBurnButton;

	AudioListView* 	fTrackList;
	SizeView*		fSizeView;

	BNotification	fNotification;
	float			fProgress;
	BString			fETAtime;
	OutputParser	fParser;

	int				fAction;
};


#endif	// _COMPILATIONAUDIOVIEW_H_
