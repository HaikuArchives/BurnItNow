/*
 * Copyright 2010-2012, BurnItNow Team. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#include "CompilationAudioView.h"
#include "CommandThread.h"

#include <Alert.h>
#include <Button.h>
#include <ControlLook.h>
#include <Entry.h>
#include <LayoutBuilder.h>
#include <ScrollView.h>
#include <String.h>
#include <StringView.h>

static const float kControlPadding = be_control_look->DefaultItemSpacing();

// Message constants
const int32 kBurnerMessage = 'Brnr';
const int32 kBurnDiscMessage = 'BURN';

CompilationAudioView::CompilationAudioView(BurnWindow& parent)
	:
	BView("Audio", B_WILL_DRAW, new BGroupLayout(B_VERTICAL, kControlPadding)),
	fBurnerThread(NULL),
	fTrackPath(new BPath())
{
	windowParent = &parent;
	fCurrentPath = 0;
	
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	fBurnerInfoBox = new BSeparatorView(B_HORIZONTAL, B_FANCY_BORDER);
	fBurnerInfoBox->SetFont(be_bold_font);
	fBurnerInfoBox->SetLabel("Ready");

	fBurnerInfoTextView = new BTextView("AudioInfoTextView");
	fBurnerInfoTextView->SetWordWrap(false);
	fBurnerInfoTextView->MakeEditable(false);
	BScrollView* infoScrollView = new BScrollView("AudioInfoScrollView",
		fBurnerInfoTextView, 0, true, true);

	BButton* burnDiscButton = new BButton("BurnDiscButton",
		"Burn disc", new BMessage(kBurnDiscMessage));
	burnDiscButton->SetTarget(this);
	burnDiscButton->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));

	fAudioBox = new BSeparatorView(B_HORIZONTAL, B_FANCY_BORDER);
	fAudioBox->SetFont(be_bold_font);
	fAudioBox->SetLabel("Drop tracks here (only WAV files)");

	fAudioList = new BListView("AudioListView");
	BScrollView* audioScrollView = new BScrollView("AudioScrollView",
		fAudioList, 0, true, true);
	
	fAudioList->AddItem(new BStringItem("<list is empty>"));

	BLayoutBuilder::Group<>(dynamic_cast<BGroupLayout*>(GetLayout()))
		.SetInsets(kControlPadding)
		.AddGroup(B_HORIZONTAL)
			.AddGlue()
			.Add(burnDiscButton)
			.AddGlue()
			.End()
		.AddGroup(B_HORIZONTAL)
			.AddGroup(B_VERTICAL)
				.Add(fBurnerInfoBox)
				.Add(infoScrollView)
				.End()
			.AddGroup(B_VERTICAL)
				.Add(fAudioBox)
				.Add(audioScrollView)
				.End()
			.End();
}


CompilationAudioView::~CompilationAudioView()
{
	delete fBurnerThread;
	delete fTrackPath;
}


#pragma mark -- BView Overrides --


void
CompilationAudioView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kBurnerMessage:
			_BurnerParserOutput(message);
			break;
		case kBurnDiscMessage:
			BurnDisc();
			break;
		case B_REFS_RECEIVED:
			_AddTrack(message);
			break;
		default:
			BView::MessageReceived(message);
	}
}


#pragma mark -- Private Methods --


void
CompilationAudioView::_BurnerParserOutput(BMessage* message)
{
	BString data;

	if (message->FindString("line", &data) != B_OK)
		return;

	data << "\n";

	fBurnerInfoTextView->Insert(data.String());
	
	if (!fBurnerThread->IsRunning())
		fBurnerInfoBox->SetLabel("Ready");
}


void
CompilationAudioView::_AddTrack(BMessage* message)
{
	// TODO Verify that the file is a WAV file
	entry_ref trackRef;

	if (message->FindRef("refs", &trackRef) != B_OK)
		return;
	
	if (fCurrentPath == 0) {
		// fAudioList->RemoveItem(0) returns error
		int32 tmp = 0;
		fAudioList->RemoveItem(tmp);
	}
	
	fTrackPath->SetTo(&trackRef);
	fTrackPaths[fCurrentPath++] = fTrackPath;

	BStringItem* item = new BStringItem(fTrackPath->Leaf());
	fAudioList->AddItem(item);
}


#pragma mark -- Public Methods --


void
CompilationAudioView::BurnDisc()
{
	if (fAudioList->IsEmpty())
		return;

	fBurnerInfoTextView->SetText(NULL);
	fBurnerInfoBox->SetLabel("Burning in progress" B_UTF8_ELLIPSIS);
	BString device = windowParent->GetSelectedDevice().number.String();
	
	fBurnerThread = new CommandThread(NULL,
		new BInvoker(new BMessage(kBurnerMessage), this));
	
	fBurnerThread->AddArgument("cdrecord")
		->AddArgument("dev=")
		->AddArgument(device)
		->AddArgument("-audio")
		->AddArgument("-pad");
	
	if (windowParent->GetSessionMode())
		fBurnerThread->AddArgument("-sao");
	else
		fBurnerThread->AddArgument("-tao");
		
	for (unsigned int ix = 0; ix<=MAX_TRACKS; ix++) {
		if (fTrackPaths[ix] == NULL)
			break;
		fBurnerThread->AddArgument(fTrackPaths[ix]->Path());
	}
	
	fBurnerThread->Run();
}
