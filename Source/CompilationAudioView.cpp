/*
 * Copyright 2010-2012, BurnItNow Team. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#include "CompilationAudioView.h"

#include "CommandThread.h"

#include <Alert.h>
#include <Button.h>
#include <LayoutBuilder.h>
#include <Path.h>
#include <ScrollView.h>
#include <String.h>
#include <StringView.h>

const float kControlPadding = 5;

// Message constants
const int32 kBurnerMessage = 'Brnr';

CompilationAudioView::CompilationAudioView(BurnWindow &parent)
	:
	BView("Audio", B_WILL_DRAW, new BGroupLayout(B_VERTICAL, kControlPadding)),
	fBurnerThread(NULL)
{
	windowParent = &parent;
	
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	fBurnerInfoBox = new BBox("AudioInfoBBox");
	fBurnerInfoBox->SetLabel("Ready.");

	fBurnerInfoTextView = new BTextView("AudioInfoTextView");
	fBurnerInfoTextView->SetWordWrap(false);
	fBurnerInfoTextView->MakeEditable(false);
	BScrollView* infoScrollView = new BScrollView("AudioInfoScrollView", fBurnerInfoTextView, 0, true, true);

	fBurnerInfoBox->AddChild(infoScrollView);
	
	fAudioBox = new BBox("AudioInfoBBox");
	fAudioBox->SetLabel("Drop tracks here.");

	fAudioList = new BListView("AudioListView");
	BScrollView* audioScrollView = new BScrollView("AudioScrollView", fAudioList, 0, true, true);

	fAudioBox->AddChild(audioScrollView);
	
	fAudioList->AddItem(new BStringItem("<list is empty>"));

	BLayoutBuilder::Group<>(dynamic_cast<BGroupLayout*>(GetLayout()))
		.SetInsets(kControlPadding, kControlPadding, kControlPadding, kControlPadding)
		.AddGroup(B_HORIZONTAL)
			.Add(fBurnerInfoBox)
			.Add(fAudioBox)
			.End();
}


CompilationAudioView::~CompilationAudioView()
{
	delete fBurnerThread;
}


#pragma mark -- BView Overrides --


void CompilationAudioView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kBurnerMessage:
			_BurnerParserOutput(message);
			break;
		default:
			BView::MessageReceived(message);
	}
}


#pragma mark -- Private Methods --


void CompilationAudioView::_BurnerParserOutput(BMessage* message)
{
	BString data;

	if (message->FindString("line", &data) != B_OK)
		return;

	data << "\n";

	fBurnerInfoTextView->Insert(data.String());
	
	if (!fBurnerThread->IsRunning())
		fBurnerInfoBox->SetLabel("Ready.");
}
