/*
 * Copyright 2010-2017, BurnItNow Team. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _COMPILATIONAUDIOVIEW_H_
#define _COMPILATIONAUDIOVIEW_H_

#include <Button.h>
#include <FilePanel.h>
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


class AudioRefFilter : public BRefFilter {
public:
					AudioRefFilter() {};
	virtual			~AudioRefFilter() {};

	bool			Filter(const entry_ref* ref, BNode* node,
						struct stat_beos* st, const char* filetype);
};


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
	void			_UpdateButtons();
	void			_UpdateProgress();
	void			_UpdateSizeBar();
	
	CommandThread*	fBurnerThread;
	BurnWindow* 	fWindowParent;

	BFilePanel* 	fOpenPanel;
	BTextView* 		fOutputView;
	BSeparatorView*	fInfoView;
	BSeparatorView*	fAudioBox;
	BButton*		fBurnButton;

	BButton*		fUpButton;
	BButton*		fDownButton;
	BButton*		fPlayButton;
	BButton*		fAddButton;
	BButton*		fRemoveButton;

	AudioListView* 	fTrackList;
	SizeView*		fSizeView;

	BNotification	fNotification;
	float			fProgress;
	BString			fETAtime;
	OutputParser	fParser;

	int32			fAbort;
	int32			fAction;
};


#endif	// _COMPILATIONAUDIOVIEW_H_
